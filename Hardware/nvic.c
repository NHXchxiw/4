#include "nvic.h"



/*
 * NVIC优先级配置
 * 注意：STM32F401使用4位抢占优先级(0-15)，无子优先级
 * PendSV必须设为最低优先级15，SysTick设为14
 * 其他中断优先级必须高于PendSV(即数值小于15)
 */
void NVIC_PriorityConfig(void)
{
    // 设置优先级分组为4位抢占优先级
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    
    // 设置PendSV为最低优先级15
    NVIC_SetPriority(PendSV_IRQn, 15);
    
    // 设置SysTick为14
    NVIC_SetPriority(SysTick_IRQn, 14);
    
    // TIM2捕获中断（PPM输入）优先级设为1
    NVIC_SetPriority(TIM1_CC_IRQn, 1);
}
