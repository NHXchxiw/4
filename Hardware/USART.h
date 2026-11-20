#ifndef HARDWARE_SERIAL_USART_H
#define HARDWARE_SERIAL_USART_H

#include "stm32f4xx.h"

#define SERIAL_BAUDRATE 115200

/* 1: temporary ASCII heartbeat/echo test; 0: normal ANO attitude mode.
 * The normal baud rate above is preserved when leaving the test. */
#define SERIAL_SELF_TEST_EN        0u
#define SERIAL_SELF_TEST_BAUDRATE  115200u

/* Wired serial on USART1 PB6/PB7; initialize after ANO_Protocol_Init(). */
void Serial_Init(void);
/* Task context, single sender (TaskGY86). Copies the complete binary frame
 * to USART1; returns 0 on timeout, 1 after all bytes enter the peripheral.
 * Do not mix BLE/printf text with ANO frames on this port. */
uint8_t Serial_SendBuffer(const uint8_t *data, uint16_t length);

#if SERIAL_SELF_TEST_EN
typedef struct
{
    uint32_t irq_count;
    uint32_t rx_bytes;          /* Includes bytes accompanied by UART errors. */
    uint32_t rx_errors;
    uint32_t rx_overflows;
    uint32_t echo_bytes;        /* Bytes submitted for echo, not PC acknowledgments. */
    uint32_t heartbeat_count;
    uint32_t tx_errors;
    uint32_t last_error_flags;  /* USART SR error bits; captured in the ISR. */
    uint8_t last_rx_byte;
} Serial_Test_Diagnostics;

extern volatile Serial_Test_Diagnostics Serial_Test_Debug;
/* Single task calls this every 10 ms; the ISR only queues received bytes. */
void Serial_SelfTest_Process(void);
#endif

#endif
