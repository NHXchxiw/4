#include "stm32f4xx.h"
#include "ucos_ii.h"
#include "control_timer.h"
#include "delay.h"
#include <string.h>

#if CONTROL_TIMER_IRQ_PRIO < CPU_CFG_KA_IPL_BOUNDARY || CONTROL_TIMER_IRQ_PRIO >= 15u
#error "TIM2 must use a kernel-aware interrupt priority above PendSV"
#endif
#if (CONTROL_INNER_HZ % CONTROL_OUTER_HZ) != 0u
#error "The inner loop frequency must be a multiple of the outer frequency"
#endif

volatile ControlTimer_DebugData ControlTimer_Debug;
static OS_EVENT *ControlSem;//	中断与控制任务之间的信号量
static volatile uint8_t WakePending;// 是否已有一次尚未被任务消费的周期通知
static uint8_t Initialized, HaveWake;// HaveWake是否已经记录过第一次任务唤醒时间
static uint32_t LastWakeCycles, CyclesPerUs;

uint8_t ControlTimer_Init(void)
{
    RCC_ClocksTypeDef clocks;
    TIM_TimeBaseInitTypeDef timer = {0};
    NVIC_InitTypeDef nvic = {0};
    uint32_t timer_clock;

    if (Initialized) return 1u;//已初始化就返回，防止重复创建信号量
    RCC_GetClocksFreq(&clocks);
    timer_clock = clocks.PCLK1_Frequency;
	
	//CFGR寄存器中的[12：10]位表示APB1的实际分频，标准库设置“不分频”时写入 000，设置分频时写入 100～111
	//CFGR与该[12：10]位掩码(PPRE1)与则把这三位以外清0，分频即不为零，得到2*APB1=84Mhz
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0u) timer_clock *= 2u;
	
    if (timer_clock < 1000000u || timer_clock % 1000000u != 0u) return 0u;//检查是否能整数预分频
    CyclesPerUs = SystemCoreClock / 1000000u;
    if (CyclesPerUs == 0u) return 0u;
    ControlSem = OSSemCreate(0u);//创建初始计数为0信号量，表示目前没有达到任何一个待执行周期
    if (ControlSem == (OS_EVENT *)0) return 0u;
    Delay_Init();

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);//允许tim2获取外设时钟
    TIM_DeInit(TIM2);
    timer.TIM_Prescaler = (uint16_t)(timer_clock / 1000000u - 1u);
    timer.TIM_Period = CONTROL_PERIOD_US - 1u;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &timer);
    TIM_ARRPreloadConfig(TIM2, ENABLE);//预装载，防止在计数周期内突然改变周期
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);//人为更新，让配置立刻生效
	
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);//人为更新可能导致PIF置位，如果留下这个标志稍后开启中断可能会马上进入一次中断（但是这次中断并不是计数器1ms引起的），故在此清中断
    NVIC_ClearPendingIRQ(TIM2_IRQn);

    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = CONTROL_TIMER_IRQ_PRIO;
    nvic.NVIC_IRQChannelSubPriority = 0u;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    Initialized = 1u;
    return 1u;
}

void ControlTimer_Start(void)
{
    OS_CPU_SR cpu_sr;
    if (!Initialized) return;
    OS_ENTER_CRITICAL();
    TIM_Cmd(TIM2, DISABLE);//停止计数
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);//停止更新中断
    while (OSSemAccept(ControlSem) != 0u) { }
    memset((void *)&ControlTimer_Debug, 0, sizeof(ControlTimer_Debug));
    WakePending = HaveWake = 0u;
    TIM_SetCounter(TIM2, 0u);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    ControlTimer_Debug.running = 1u;
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    OS_EXIT_CRITICAL();
}

void ControlTimer_Stop(void)
{
    OS_CPU_SR cpu_sr;
    if (!Initialized) return;
    OS_ENTER_CRITICAL();
    TIM_Cmd(TIM2, DISABLE);
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    ControlTimer_Debug.running = 0u;
    OS_EXIT_CRITICAL();
}

uint8_t ControlTimer_Wait(void)
{
    OS_CPU_SR cpu_sr;
    INT8U err;
    uint32_t now, elapsed;

    if (!Initialized || !ControlTimer_Debug.running) return 0u;
    now = DWT->CYCCNT;
    if (HaveWake)
    {
        elapsed = (uint32_t)(now - LastWakeCycles) / CyclesPerUs;
        ControlTimer_Debug.last_work_us = elapsed;
        if (elapsed > ControlTimer_Debug.max_work_us) ControlTimer_Debug.max_work_us = elapsed;
        if (elapsed > CONTROL_PERIOD_US) ++ControlTimer_Debug.overrun_count;
    }
    OSSemPend(ControlSem, 0u, &err);
    if (err != OS_ERR_NONE) return 0u;
    OS_ENTER_CRITICAL();
    WakePending = 0u;
    ++ControlTimer_Debug.wake_count;
    now = DWT->CYCCNT;
    if (HaveWake)
    {
        elapsed = (uint32_t)(now - LastWakeCycles) / CyclesPerUs;
        ControlTimer_Debug.last_interval_us = elapsed;
        if (elapsed > ControlTimer_Debug.max_interval_us) ControlTimer_Debug.max_interval_us = elapsed;
    }
    LastWakeCycles = now;
    HaveWake = 1u;
    OS_EXIT_CRITICAL();
    return 1u;
}

void TIM2_IRQHandler(void)
{
    OSIntEnter();
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        ++ControlTimer_Debug.irq_count;
        if (ControlTimer_Debug.running)
        {
            if (WakePending)
                ++ControlTimer_Debug.coalesced_ticks;
            else
            {
                WakePending = 1u;
                if (OSSemPost(ControlSem) != OS_ERR_NONE)
                {
                    WakePending = 0u;
                    ++ControlTimer_Debug.post_errors;
                }
            }
        }
    }
    OSIntExit();
}
