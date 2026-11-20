#ifndef _USART1_H
#define _USART1_H

#include "stm32f4xx.h"

/* Standalone Bluetooth initialization, USART1 PB6/PB7, 115200 8N1.
 * This file and buletooth.c are excluded from the wired target's build.
 * This is not the unused PA9/PA10 driver in System/usart1.h. */
void USART1_Init(void);

#endif
