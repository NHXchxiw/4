#include "stm32f4xx.h"
#include "delay.h"
#include "MYI2C.h"

#define MYI2C_TIMEOUT_US 1000u
#define MYI2C_ERROR_MASK (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR)

static uint32_t TransactionStart;

static uint8_t MYI2C_TimedOut(void)
{
    return (uint32_t)(DWT->CYCCNT - TransactionStart) >=
           (SystemCoreClock / 1000000u) * MYI2C_TIMEOUT_US;
}

static uint8_t MYI2C_Wait(uint32_t mask)
{
    while ((I2C1->SR1 & mask) == 0u)
    {
        if ((I2C1->SR1 & MYI2C_ERROR_MASK) != 0u || MYI2C_TimedOut()) return 0u;
    }
    return (I2C1->SR1 & MYI2C_ERROR_MASK) == 0u;
}

static void MYI2C_ClearAddress(void)
{
    volatile uint32_t discard;
    discard = I2C1->SR1;
    discard = I2C1->SR2;
    (void)discard;
}

static void MYI2C_Configure(void)
{
    GPIO_InitTypeDef gpio = {0};
    I2C_InitTypeDef i2c = {0};

    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_OD;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_I2C1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);
    GPIO_Init(GPIOB, &gpio);

    I2C_DeInit(I2C1);
    i2c.I2C_ClockSpeed = MYI2C_CLOCK_HZ;
    i2c.I2C_Mode = I2C_Mode_I2C;
    i2c.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1 = 0u;
    i2c.I2C_Ack = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &i2c);
    I2C_Cmd(I2C1, ENABLE);
}

uint8_t MYI2C_Recover(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t i, released;

    I2C_Cmd(I2C1, DISABLE);
    GPIO_SetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9);
    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_OD;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    Delay_us(3u);
    for (i = 0u; i < 9u && GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9) == Bit_RESET; ++i)
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_8);
        Delay_us(3u);
        GPIO_SetBits(GPIOB, GPIO_Pin_8);
        Delay_us(3u);
    }
    /* STOP releases a slave left partway through an earlier transaction. */
    GPIO_ResetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9);
    Delay_us(3u);
    GPIO_SetBits(GPIOB, GPIO_Pin_8);
    Delay_us(3u);
    GPIO_SetBits(GPIOB, GPIO_Pin_9);
    Delay_us(3u);
    released = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8) == Bit_SET &&
               GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9) == Bit_SET;
    MYI2C_Configure();
    return released;
}

void MYI2C_Init(void)
{
    Delay_Init();
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    (void)MYI2C_Recover();
}

static uint8_t MYI2C_Fail(void)
{
    /* Do not retry a partially read sample. Restore the bus for the next call. */
    if ((I2C1->SR2 & I2C_SR2_MSL) != 0u) I2C_GenerateSTOP(I2C1, ENABLE);
    (void)MYI2C_Recover();
    return 0u;
}

static uint8_t MYI2C_Begin(uint8_t address, uint8_t reg)
{
    TransactionStart = DWT->CYCCNT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) != 0u || (I2C1->CR1 & I2C_CR1_STOP) != 0u)
    {
        if (MYI2C_TimedOut()) return 0u;
    }
    I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!MYI2C_Wait(I2C_SR1_SB)) return 0u;
    I2C_Send7bitAddress(I2C1, (uint8_t)(address << 1), I2C_Direction_Transmitter);
    if (!MYI2C_Wait(I2C_SR1_ADDR)) return 0u;
    MYI2C_ClearAddress();
    if (!MYI2C_Wait(I2C_SR1_TXE)) return 0u;
    I2C_SendData(I2C1, reg);
    return MYI2C_Wait(I2C_SR1_BTF);
}

uint8_t MYI2C_WriteReg(uint8_t address, uint8_t reg, uint8_t value)
{
    if (!MYI2C_Begin(address, reg)) return MYI2C_Fail();
    I2C_SendData(I2C1, value);
    if (!MYI2C_Wait(I2C_SR1_BTF)) return MYI2C_Fail();
    I2C_GenerateSTOP(I2C1, ENABLE);
    return 1u;
}

uint8_t MYI2C_ReadRegs(uint8_t address, uint8_t reg, uint8_t *data, uint8_t length)
{
    uint32_t primask;
    uint8_t remaining = length;

    if (data == 0 || length == 0u) return 0u;
    if (!MYI2C_Begin(address, reg)) return MYI2C_Fail();
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!MYI2C_Wait(I2C_SR1_SB)) return MYI2C_Fail();
    if (length == 2u) I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Next);
    I2C_Send7bitAddress(I2C1, (uint8_t)(address << 1), I2C_Direction_Receiver);
    if (!MYI2C_Wait(I2C_SR1_ADDR)) return MYI2C_Fail();

    /* F401 I2C needs distinct ACK/POS/STOP sequences for 1, 2 and >=3 bytes.
     * Mask interrupts only for short register sequences, never for polling. */
    if (length == 1u)
    {
        primask = __get_PRIMASK();
        __disable_irq();
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        MYI2C_ClearAddress();
        I2C_GenerateSTOP(I2C1, ENABLE);
        __set_PRIMASK(primask);
        if (!MYI2C_Wait(I2C_SR1_RXNE)) return MYI2C_Fail();
        *data = I2C_ReceiveData(I2C1);
    }
    else if (length == 2u)
    {
        primask = __get_PRIMASK();
        __disable_irq();
        MYI2C_ClearAddress();
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        __set_PRIMASK(primask);
        if (!MYI2C_Wait(I2C_SR1_BTF)) return MYI2C_Fail();
        primask = __get_PRIMASK();
        __disable_irq();
        I2C_GenerateSTOP(I2C1, ENABLE);
        *data++ = I2C_ReceiveData(I2C1);
        *data = I2C_ReceiveData(I2C1);
        __set_PRIMASK(primask);
    }
    else
    {
        MYI2C_ClearAddress();
        while (remaining > 3u)
        {
            if (!MYI2C_Wait(I2C_SR1_RXNE)) return MYI2C_Fail();
            *data++ = I2C_ReceiveData(I2C1);
            --remaining;
        }
        if (!MYI2C_Wait(I2C_SR1_BTF)) return MYI2C_Fail();
        primask = __get_PRIMASK();
        __disable_irq();
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        *data++ = I2C_ReceiveData(I2C1);
        __set_PRIMASK(primask);
        if (!MYI2C_Wait(I2C_SR1_BTF)) return MYI2C_Fail();
        primask = __get_PRIMASK();
        __disable_irq();
        I2C_GenerateSTOP(I2C1, ENABLE);
        *data++ = I2C_ReceiveData(I2C1);
        *data = I2C_ReceiveData(I2C1);
        __set_PRIMASK(primask);
    }
    /* Restore ACK/POS in the next Begin(), after STOP has completed. */
    return 1u;
}
