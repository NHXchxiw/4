#include "stm32f4xx.h"                  // Device header
#include "ucos_ii.h"
#include "BSP.h"
#include "app.h"

int main(void)
{
    BSP_Init();

    OSInit();
    AppTaskCreate();
    OSStart();
    
	while(1)
	{
        
	}
}
