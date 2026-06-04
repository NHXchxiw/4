#include "stm32f4xx.h"                  // Device header
#include "BSP.h"


void BSP_Init(void)
{
    NVIC_PriorityConfig();         // 配置NVIC优先级
	TIM1_PPM_Init();
	PWM_Init();
    MYGPIO_Init();                 
	MYI2C_Init();
	USART1_Init();
}
