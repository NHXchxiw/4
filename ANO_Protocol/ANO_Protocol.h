#ifndef __ANO_PROTOCOL_H
#define __ANO_PROTOCOL_H

#include <stdint.h>
#include "attitude_control.h"

/* ANO TC address selection: configure the upper computer to address 0x05. */
#define ANO_DEVICE_ADDRESS       0x05u
#define ANO_HOST_ADDRESS         0xAFu
#define ANO_BROADCAST_ADDRESS    0xFFu
#define ANO_RX_CAPTURE_SIZE      32u

/* PID_1_P is parameter 11 in the supplied ANO TC parameter table.
 * This project maps parameters 11..19 to PID groups 1..3, P/I/D in order.
 * The current project has no flight PID loop yet; the loop must consume these. */
#define ANO_PARAM_PID_1_P        11u
#define ANO_PARAM_PID_1_I        12u
#define ANO_PARAM_PID_1_D        13u
#define ANO_PARAM_PID_2_P        14u
#define ANO_PARAM_PID_2_I        15u
#define ANO_PARAM_PID_2_D        16u
#define ANO_PARAM_PID_3_P        17u
#define ANO_PARAM_PID_3_I        18u
#define ANO_PARAM_PID_3_D        19u

typedef struct
{
    uint16_t p;
    uint16_t i;
    uint16_t d;
} ANO_PID_Config;

/* PID_1, PID_2, PID_3 parameter values, in the units configured by ANO TC. */
extern volatile ANO_PID_Config ANO_PID_Groups[3];

/* Keil Watch: successful traffic and reasons for discarded data.
 * TX success means queued to USART1, not acknowledged by the PC. */
typedef struct
{
    uint32_t rx_frames;
    uint32_t checksum_errors;
    uint32_t length_errors;
    uint32_t rx_overflows;
    uint32_t uart_errors;
    uint32_t tx_frames;
    uint32_t tx_errors;
    uint32_t rx_bytes;            /* Bytes delivered by the UART ISR, before parsing. */
    uint32_t address_errors;      /* AA header followed by a nonlocal address. */
    uint8_t last_rx_byte;
    uint8_t last_address;         /* Latest candidate AA header's destination. */
    uint8_t last_id;              /* Candidate ID/length when supplied by the stream. */
    uint8_t last_length;
    uint8_t rx_capture_count;
    uint8_t rx_capture[ANO_RX_CAPTURE_SIZE]; /* First bytes since Init, not a live ring. */
} ANO_Protocol_Diagnostics;

extern volatile ANO_Protocol_Diagnostics ANO_Protocol_Debug;

/* V7 frame: AA D_ADDR ID LEN DATA SC AC; multibyte DATA is little endian.
 * SC sums AA through DATA; AC sums each running SC, both modulo 256.
 * Wired USART.c owns RX interrupts and binary TX; no BLE transport is used. */
void ANO_Protocol_Init(void);
/* Called once per received byte from USART1_IRQHandler. */
void ANO_Protocol_RxByte(uint8_t byte);
/* ISR reports ORE/FE/NE/PE; discard the interrupted stream before parsing. */
void ANO_Protocol_RxError(void);
/* Called by the attitude task, outside the I2C/filter scheduler lock. */
void ANO_Protocol_Process(uint32_t now_us);
void ANO_Protocol_SendTelemetry(const Attitude_Data *data);
uint8_t ANO_Protocol_GetPID(uint8_t group, ANO_PID_Config *config);

#endif
