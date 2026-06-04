/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                      APPLICATION CONFIGURATION
*
*                                            EXAMPLE CODE
*
* Filename : app_cfg.h
*********************************************************************************************************
*/

#ifndef  _APP_CFG_H_
#define  _APP_CFG_H_


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

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
#define BLUETOOTH_EN 1


#if LED_EN
#include "led.h"
#endif


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
#endif

#if IMU_EN
#include "IMU.h"
#endif

#if DELAY_EN
#include "delay.h"
#endif

#if PID_EN
#include "pid.h"
#endif

#if BLUETOOTH_EN
#include "bluetooth.h"
#endif

/*
*********************************************************************************************************
*                                       MODULE ENABLE / DISABLE
*********************************************************************************************************
*/
//#define  __TARGET_FPU_SOFTVFP    0u

/*
*********************************************************************************************************
*                                           TASK PRIORITIES
*********************************************************************************************************
*/

#define  APP_CFG_STARTUP_TASK_PRIO          3u

#define  OS_TASK_TMR_PRIO                  (OS_LOWEST_PRIO - 2u)

/*
*********************************************************************************************************
*                                   CPU CONFIGURATION
*********************************************************************************************************
*/

#define  CPU_CFG_KA_IPL_BOUNDARY            5u   /* 中断优先级边界，值越大优先级越低 (0-15) */
#define  CPU_CFG_NVIC_PRIO_BITS             4u   /* STM32 使用 4 位优先级 */


/*
*********************************************************************************************************
*                                          TASK STACK SIZES
*                             Size of the task stacks (# of OS_STK entries)
*********************************************************************************************************
*/

#define  APP_CFG_STARTUP_TASK_STK_SIZE    128u


/*
*********************************************************************************************************
*                                     TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                    0u
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                   1u
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                    2u
#endif

#define  APP_TRACE_LEVEL                   TRACE_LEVEL_OFF
#define  APP_TRACE                         printf

#define  APP_TRACE_INFO(x)    ((APP_TRACE_LEVEL >= TRACE_LEVEL_INFO)  ? (void)(APP_TRACE x) : (void)0)
#define  APP_TRACE_DBG(x)     ((APP_TRACE_LEVEL >= TRACE_LEVEL_DBG)   ? (void)(APP_TRACE x) : (void)0)


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif                                                          /* End of module include.              */
