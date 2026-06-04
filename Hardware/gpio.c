#include "stm32f4xx.h"
#include "gpio.h"

void MYGPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 仅开启 GPIOA 时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    
    // 2. 配置 PA5 (LD2) 为推挽输出
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin   = BLED_Pin;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BLED_GPIO_Port, &GPIO_InitStructure);
    
    // 3. 初始状态设定为低电平（灭）
    GPIO_ResetBits(BLED_GPIO_Port, BLED_Pin);
}
