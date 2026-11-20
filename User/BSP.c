#include "stm32f4xx.h"                  // Device header
#include "BSP.h"
#include "ANO_Protocol.h"
#include "USART.h"


void BSP_Init(void)
{
    NVIC_PriorityConfig();         // 配置NVIC优先级
//	TIM1_PPM_Init();
//	PWM_Init();
    MYGPIO_Init();                 
	ANO_Protocol_Init();
	Serial_Init();                 // USART1 PB6/PB7: wired ANO TC port
}
