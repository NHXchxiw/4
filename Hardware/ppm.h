#ifndef __ppm_h
#define __ppm_h

#include "stm32f4xx.h"                  // Device header
#include "ucos_ii.h"
// PPM相关变量


extern volatile uint16_t ppm_channels[8];
extern volatile uint8_t ppm_frame_ready;
extern volatile uint8_t frame_channel_count;
extern uint8_t throttle_channel; // 默认通道3
//extern volatile uint32_t ppm_interrupt_count;
extern volatile uint8_t  ch_idx;

void TIM1_PPM_Init(void);
void TIM1_CC_IRQHandler(void);  // 改为TIM1的中断处理函数


#endif
