#include "pwm.h"


void PWM_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // F4系列时钟使能：TIM3在APB1，GPIOC在AHB1
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    // 配置PC6为TIM3_CH1输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;          // 复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;    // F4支持更高速度
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;        // 推挽输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;      // 无上下拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 配置PC6复用为TIM3_CH1
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);

    TIM_Cmd(TIM3, DISABLE);
    
    // 定时器配置：50Hz PWM (20ms周期)
    // F401: APB1定时器时钟为84MHz
    TIM_TimeBaseStructure.TIM_Period = 19999;            // 20000-1
    TIM_TimeBaseStructure.TIM_Prescaler = 83;            // 84MHz/84 = 1MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;      // 高级定时器特有，TIM3无此寄存器
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // PWM输出配置
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;  // 互补输出禁用
    TIM_OCInitStructure.TIM_Pulse = 1000;                // 初始占空比1ms
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    
    // 启动定时器
    TIM_Cmd(TIM3, ENABLE);
}

void ESC_SetThrottle(uint16_t pulse_us) {
    if(pulse_us < ESC_PULSE_MIN) pulse_us = ESC_PULSE_MIN;
    if(pulse_us > ESC_PULSE_MAX) pulse_us = ESC_PULSE_MAX;
    TIM3->CCR1 = pulse_us;  // 使用TIM3的CCR1寄存器
}
