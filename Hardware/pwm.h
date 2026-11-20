#ifndef __pwm_h
#define __pwm_h

#include "stm32f4xx.h"
#include "ucos_ii.h"

#define ESC_CHANNEL_COUNT  4u
#define ESC_PWM_HZ         50u
#define ESC_PULSE_MIN      900u
#define ESC_PULSE_MAX      2100u
#define ESC_PULSE_STOP     1000u

/* TIM3_CH1..4 = PC6, PC7, PC8, PC9; 1 timer count = 1 us.
 * Initializes all four outputs to ESC_PULSE_STOP. No throttle calibration. */
void PWM_Init(void);

/* Existing single-motor API: channel 1 (PC6) only. */
void ESC_SetThrottle(uint16_t pulse_us);
/* Channel numbers are 1..4, matching the schematic PWM3/1..PWM3/4.
 * Invalid channel numbers are ignored. All pulse setters clamp to MIN..MAX.
 * Call from task context; new values take effect at the next PWM period. */
void ESC_SetThrottleChannel(uint8_t channel, uint16_t pulse_us);
/* Set all four motors to the same pulse width. */
void ESC_SetThrottleAll(uint16_t pulse_us);
/* Update four different pulse widths together at the same period boundary. */
void ESC_SetThrottles(uint16_t motor1_us, uint16_t motor2_us,
                      uint16_t motor3_us, uint16_t motor4_us);
void ESC_StopAll(void);

#endif
