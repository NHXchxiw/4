#include "BSP.h"
#include "app.h"

int main(void)
{
    /* Enable Cortex-M4F context preservation before the OS initializes. */
    SCB->CPACR |= (3UL << 20) | (3UL << 22);
    __DSB();
    __ISB();
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;

    OSInit();
		
    TaskCreate();
		
    OSStart();
    
	while(1)
	{
        
	}
}
