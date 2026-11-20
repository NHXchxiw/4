#include "pwm.h"

static uint32_t PWM_TimerClockHz(void)
{
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);
    /* TIMPRE=0: PCLK1 or 2*PCLK1. TIMPRE=1: HCLK up to APB /4,
     * otherwise 4*PCLK1. Do not assume the APB bus clock is 84 MHz. */
    if ((RCC->DCKCFGR & RCC_DCKCFGR_TIMPRE) != 0u)
        return clocks.HCLK_Frequency / clocks.PCLK1_Frequency <= 4u
             ? clocks.HCLK_Frequency : 4u * clocks.PCLK1_Frequency;
    return clocks.PCLK1_Frequency == clocks.HCLK_Frequency
         ? clocks.PCLK1_Frequency : 2u * clocks.PCLK1_Frequency;
}

static uint16_t ESC_LimitPulse(uint16_t pulse_us)
{
    if (pulse_us < ESC_PULSE_MIN) pulse_us = ESC_PULSE_MIN;
    if (pulse_us > ESC_PULSE_MAX) pulse_us = ESC_PULSE_MAX;
    return pulse_us;
}

void PWM_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_TimeBaseInitTypeDef timer = {0};
    TIM_OCInitTypeDef output = {0};

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_DeInit(TIM3);

    timer.TIM_Period = 1000000u / ESC_PWM_HZ - 1u;
    timer.TIM_Prescaler = (uint16_t)(PWM_TimerClockHz() / 1000000u - 1u);
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_RepetitionCounter = 0u;
    TIM_TimeBaseInit(TIM3, &timer);

    output.TIM_OCMode = TIM_OCMode_PWM1;
    output.TIM_OutputState = TIM_OutputState_Enable;
    output.TIM_OutputNState = TIM_OutputNState_Disable;
    output.TIM_Pulse = ESC_PULSE_STOP;
    output.TIM_OCPolarity = TIM_OCPolarity_High;
    output.TIM_OCNPolarity = TIM_OCNPolarity_High;
    output.TIM_OCIdleState = TIM_OCIdleState_Reset;
    output.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(TIM3, &output);
    TIM_OC2Init(TIM3, &output);
    TIM_OC3Init(TIM3, &output);
    TIM_OC4Init(TIM3, &output);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_GenerateEvent(TIM3, TIM_EventSource_Update);
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    TIM_SetCounter(TIM3, 0u);

    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOC, &gpio);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM3);
    TIM_Cmd(TIM3, ENABLE);
}

void ESC_SetThrottleChannel(uint8_t channel, uint16_t pulse_us)
{
    pulse_us = ESC_LimitPulse(pulse_us);
    switch (channel)
    {
        case 1u: TIM_SetCompare1(TIM3, pulse_us); break;
        case 2u: TIM_SetCompare2(TIM3, pulse_us); break;
        case 3u: TIM_SetCompare3(TIM3, pulse_us); break;
        case 4u: TIM_SetCompare4(TIM3, pulse_us); break;
        default: break;
    }
}

void ESC_SetThrottle(uint16_t pulse_us)
{
    ESC_SetThrottleChannel(1u, pulse_us);
}

void ESC_SetThrottles(uint16_t motor1_us, uint16_t motor2_us,
                      uint16_t motor3_us, uint16_t motor4_us)
{
    uint32_t old_udis;
#if OS_CRITICAL_METHOD == 3u
    OS_CPU_SR cpu_sr;
#endif
    motor1_us = ESC_LimitPulse(motor1_us);
    motor2_us = ESC_LimitPulse(motor2_us);
    motor3_us = ESC_LimitPulse(motor3_us);
    motor4_us = ESC_LimitPulse(motor4_us);

    OS_ENTER_CRITICAL();
    /* Block hardware update events as well as task preemption while filling
     * all CCR preload registers. A timer wrap must not latch half a group. */
    old_udis = TIM3->CR1 & TIM_CR1_UDIS;
    TIM3->CR1 |= TIM_CR1_UDIS;
    TIM_SetCompare1(TIM3, motor1_us);
    TIM_SetCompare2(TIM3, motor2_us);
    TIM_SetCompare3(TIM3, motor3_us);
    TIM_SetCompare4(TIM3, motor4_us);
    if (old_udis == 0u) TIM3->CR1 &= ~TIM_CR1_UDIS;
    OS_EXIT_CRITICAL();
}

void ESC_SetThrottleAll(uint16_t pulse_us)
{
    ESC_SetThrottles(pulse_us, pulse_us, pulse_us, pulse_us);
}

void ESC_StopAll(void)
{
    ESC_SetThrottleAll(ESC_PULSE_STOP);
}
