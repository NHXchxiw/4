#include "BSP.h"
#include "app.h"

int main(void)
{
    BSP_Init();
		
    OSInit();
		
    TaskCreate();
		
    OSStart();
    
	while(1)
	{
        
	}
}
