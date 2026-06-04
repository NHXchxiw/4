#include "stm32f4xx.h"                  // Device header
#include "BSP.h"


void BSP_Init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = BLED_Pin;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BLED_GPIO_Port,&GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = Buzzer_Pin;
    GPIO_Init(Buzzer_GPIO_Port,&GPIO_InitStructure);
    
    
}
