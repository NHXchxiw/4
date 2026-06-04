#ifndef __pwm_h
#define __pwm_h

#include "stm32f4xx.h"
#include "ucos_ii.h"

#define ESC_PULSE_MIN  900
#define ESC_PULSE_MAX  2100

void PWM_Init(void);
void ESC_SetThrottle(uint16_t pulse_us);

#endif
