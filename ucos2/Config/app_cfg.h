#ifndef _APP_CFG_H_
#define  _APP_CFG_H_

#include  <stdarg.h>
#include  <stdio.h>

#define LED_EN 0
#define GPIO_EN 1
#define NVIC_EN 1
#define USART_EN 1
#define PWM_EN 1
#define PPM_EN 1
#define I2C_EN 1
#define GY86_EN 1
#define IMU_EN 0
#define DELAY_EN 1
#define PID_EN 0
#define BLUETOOTH_EN 0  /* Current target uses the wired USART.c driver. */

#if GPIO_EN
#include "gpio.h"
#endif
#if NVIC_EN
#include "nvic.h"
#endif
#if USART_EN
#include "USART.h"
#endif
#if PWM_EN
#include "pwm.h"
#endif
#if PPM_EN
#include "ppm.h"
#endif
#if I2C_EN
#include "MYI2C.h"
#endif
#if GY86_EN
#include "MPU6050.h"
#include "HMC5883L.h"
#include "attitude_control.h"
#endif
#if DELAY_EN
#include "delay.h"
#endif
#if BLUETOOTH_EN
#include "bluetooth.h"
#endif

/* The Keil target uses Cortex-M4F; the OS port saves S16-S31 per task. */
#define OS_CPU_ARM_FP_EN 1u

#define APP_CFG_STARTUP_TASK_PRIO 3u
#define OS_TASK_TMR_PRIO (OS_LOWEST_PRIO - 2u)
#define CPU_CFG_KA_IPL_BOUNDARY 5u
#define CPU_CFG_NVIC_PRIO_BITS 4u
#define APP_CFG_STARTUP_TASK_STK_SIZE 128u

#define TRACE_LEVEL_OFF 0u
#define TRACE_LEVEL_INFO 1u
#define TRACE_LEVEL_DBG 2u
#define APP_TRACE_LEVEL TRACE_LEVEL_OFF
#define APP_TRACE printf
#define APP_TRACE_INFO(x) ((APP_TRACE_LEVEL >= TRACE_LEVEL_INFO) ? (void)(APP_TRACE x) : (void)0)
#define APP_TRACE_DBG(x) ((APP_TRACE_LEVEL >= TRACE_LEVEL_DBG) ? (void)(APP_TRACE x) : (void)0)

#endif
