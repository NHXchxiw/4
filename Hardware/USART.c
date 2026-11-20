#include "USART.h"
#include "misc.h"
#include "ucos_ii.h"
#if !SERIAL_SELF_TEST_EN
#include "ANO_Protocol.h"
#endif

#define SERIAL_TX_TIMEOUT_LOOPS  200000u

#if SERIAL_SELF_TEST_EN
#define SERIAL_TEST_RX_SIZE  128u
volatile Serial_Test_Diagnostics Serial_Test_Debug;
static volatile uint8_t Serial_Test_RxBuffer[SERIAL_TEST_RX_SIZE];
static volatile uint16_t Serial_Test_RxHead;
static volatile uint16_t Serial_Test_RxTail;
static uint16_t Serial_Test_StepCount;
#endif

/* Standalone wired driver: PB6=TX, PB7=RX, 8N1.
 * No Bluetooth driver is required by this target. PB6/PB7 use USART1 AF7. */
void Serial_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    USART_InitTypeDef usart = {0};
    NVIC_InitTypeDef nvic = {0};
    volatile uint32_t discard;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    NVIC_DisableIRQ(USART1_IRQn);
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
    USART_Cmd(USART1, DISABLE);

#if SERIAL_SELF_TEST_EN
    {
        Serial_Test_Diagnostics empty = {0};
        Serial_Test_Debug = empty;
        Serial_Test_RxHead = 0u;
        Serial_Test_RxTail = 0u;
        Serial_Test_StepCount = 0u;
    }
#endif

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);

#if SERIAL_SELF_TEST_EN
    usart.USART_BaudRate = SERIAL_SELF_TEST_BAUDRATE;
#else
    usart.USART_BaudRate = SERIAL_BAUDRATE;
#endif
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);

    /* SR followed by DR clears stale RXNE/ORE/FE/NE/PE conditions. */
    discard = USART1->SR;
    discard = USART1->DR;
    (void)discard;
    NVIC_ClearPendingIRQ(USART1_IRQn);
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 6u;
    nvic.NVIC_IRQChannelSubPriority = 0u;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

uint8_t Serial_SendBuffer(const uint8_t *data, uint16_t length)
{
    uint16_t i;
    uint32_t timeout;

    if (data == 0 || length == 0u) return 0u;
    if ((USART1->CR1 & (USART_CR1_UE | USART_CR1_TE)) !=
        (USART_CR1_UE | USART_CR1_TE)) return 0u;

    for (i = 0u; i < length; i++)
    {
        timeout = SERIAL_TX_TIMEOUT_LOOPS;
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        {
            if (--timeout == 0u) return 0u;
        }
        USART_SendData(USART1, data[i]);
    }
    return 1u;
}

#if SERIAL_SELF_TEST_EN
void Serial_SelfTest_Process(void)
{
    static const uint8_t heartbeat[] = "UART TEST PB6-TX PB7-RX 115200 8N1\r\n";
    uint8_t echo[SERIAL_TEST_RX_SIZE];
    uint16_t count = 0u;
    uint16_t tail = Serial_Test_RxTail;

    /* No sensor or ANO dependency. A repeated line also tests board-to-PC TX. */
    if (Serial_Test_StepCount == 0u)
    {
        if (Serial_SendBuffer(heartbeat, (uint16_t)(sizeof(heartbeat) - 1u)))
            Serial_Test_Debug.heartbeat_count++;
        else
            Serial_Test_Debug.tx_errors++;
    }
    if (++Serial_Test_StepCount >= 100u) Serial_Test_StepCount = 0u;

    /* One ISR producer, one task consumer; never transmit inside the ISR. */
    while (count < SERIAL_TEST_RX_SIZE && tail != Serial_Test_RxHead)
    {
        echo[count++] = Serial_Test_RxBuffer[tail];
        tail = (uint16_t)((tail + 1u) % SERIAL_TEST_RX_SIZE);
        Serial_Test_RxTail = tail;
    }
    if (count != 0u)
    {
        if (Serial_SendBuffer(echo, count))
            Serial_Test_Debug.echo_bytes += count;
        else
            Serial_Test_Debug.tx_errors++;
    }
}
#endif

void USART1_IRQHandler(void)
{
    uint32_t status;
    uint8_t received;

    OSIntEnter();
#if SERIAL_SELF_TEST_EN
    Serial_Test_Debug.irq_count++;
#endif
    status = USART1->SR;
    if ((status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_NE |
                   USART_SR_FE | USART_SR_PE)) != 0u)
    {
        received = (uint8_t)USART1->DR;
#if SERIAL_SELF_TEST_EN
        if ((status & USART_SR_RXNE) != 0u)
        {
            Serial_Test_Debug.rx_bytes++;
            Serial_Test_Debug.last_rx_byte = received;
        }
        if ((status & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0u)
        {
            Serial_Test_Debug.rx_errors++;
            Serial_Test_Debug.last_error_flags = status &
                (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE);
        }
        else if ((status & USART_SR_RXNE) != 0u)
        {
            uint16_t head = Serial_Test_RxHead;
            uint16_t next = (uint16_t)((head + 1u) % SERIAL_TEST_RX_SIZE);
            if (next == Serial_Test_RxTail)
                Serial_Test_Debug.rx_overflows++;
            else
            {
                Serial_Test_RxBuffer[head] = received;
                Serial_Test_RxHead = next;
            }
        }
#else
        if ((status & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0u)
            ANO_Protocol_RxError();
        else if ((status & USART_SR_RXNE) != 0u)
            ANO_Protocol_RxByte(received);
#endif
    }
    OSIntExit();
}
