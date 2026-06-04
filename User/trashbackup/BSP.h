#ifndef __BSP_H
#define __BSP_H

/* Private defines -----------------------------------------------------------*/
#define BLED_Pin GPIO_Pin_5
#define BLED_GPIO_Port GPIOA
#define Buzzer_Pin GPIO_Pin_2
#define Buzzer_GPIO_Port GPIOD

void BSP_Init(void);

#endif
