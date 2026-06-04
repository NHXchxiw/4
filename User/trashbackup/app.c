#include "stm32f4xx.h"                  // Device header
#include "ucos_ii.h"
#include "BSP.h"

/*
*********************************************************************************************************
*                                            TASK PRIORITIES
*********************************************************************************************************
*/

#define  APP_CFG_LED_TASK_PRIO             4u
#define  APP_CFG_Buzzer_TASK_PRIO             5u




/*
*********************************************************************************************************
*                                          TASK STACK SIZES
*                             Size of the task stacks (# of OS_STK entries)
*********************************************************************************************************
*/

#define  APP_CFG_LED_TASK_STK_SIZE       128u
#define  APP_CFG_Buzzer_TASK_STK_SIZE       128u


/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static  OS_STK  AppTaskStartStk[APP_CFG_STARTUP_TASK_STK_SIZE];
static  OS_STK  AppTaskLEDStk[APP_CFG_LED_TASK_STK_SIZE];
static  OS_STK  AppTaskBuzzerStk[APP_CFG_Buzzer_TASK_STK_SIZE];



/*
*********************************************************************************************************
*                                            LOCAL FUNCTIONS
*********************************************************************************************************
*/

static  void  AppTaskStart      (void *p_arg);
static  void  AppTaskLED       (void *p_arg);
static  void  AppTaskBuzzer      (void *p_arg);



/*
*********************************************************************************************************
*                                       AppTaskCreate
*
* Description : Create application tasks.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void AppTaskCreate(void)
{ 
    OSTaskCreate(AppTaskStart,
               (void *)0,
               &AppTaskStartStk[APP_CFG_STARTUP_TASK_STK_SIZE - 1u],
               APP_CFG_STARTUP_TASK_PRIO);
            

}


/*
*********************************************************************************************************
*                                       AppTaskStart
*
* Description : Start task. Creates other tasks.
*
* Argument(s) : p_arg       Argument passed by creator task (unused).
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg)
{
    INT8U err;
    
    (void)p_arg;
    
    OS_CPU_SysTickInitFreq(SystemCoreClock);
    
#if OS_CRITICAL_METHOD == 3u
OS_CPU_SR cpu_sr;
#endif
    
    APP_TRACE_INFO(("Starting application...\r\n"));
    
    OS_ENTER_CRITICAL();   
    
    err = OSTaskCreate(AppTaskLED,
                       (void *)0,
                       &AppTaskLEDStk[APP_CFG_LED_TASK_STK_SIZE - 1u],
                       APP_CFG_LED_TASK_PRIO);
    
    if (err != OS_ERR_NONE) {
        APP_TRACE_DBG(("Failed to create LED task: %d\r\n", err));
    }
    
    err = OSTaskCreate(AppTaskBuzzer,
                       (void *)0,
                       &AppTaskBuzzerStk[APP_CFG_Buzzer_TASK_STK_SIZE - 1u],
                       APP_CFG_Buzzer_TASK_PRIO);
    
    if (err != OS_ERR_NONE) {
        APP_TRACE_DBG(("Failed to create Buzzer task: %d\r\n", err));
    }
    

    
    OS_EXIT_CRITICAL();
    
    OSTaskDel(OS_PRIO_SELF);
}


/*
*********************************************************************************************************
*                                       AppTaskLED1
*
* Description : LED1 blinking task.
*
* Argument(s) : p_arg       Argument passed by creator task (unused).
*
* Return(s)   : none.
*
* Note(s)     : Blinks LED1 at GPIOA Pin 1.
*********************************************************************************************************
*/

static void AppTaskLED(void *p_arg)
{
    (void)p_arg;
    
    while (1) {
        OSTimeDlyHMSM(0, 0,0,500);
        GPIO_SetBits(BLED_GPIO_Port,BLED_Pin);
        OSTimeDlyHMSM(0, 0,1,0);
        GPIO_ResetBits(BLED_GPIO_Port,BLED_Pin);
        OSTimeDlyHMSM(0, 0,0,500);
    }
}


/*
*********************************************************************************************************
*                                       AppTaskLED2
*
* Description : LED2 blinking task.
*
* Argument(s) : p_arg       Argument passed by creator task (unused).
*
* Return(s)   : none.
*
* Note(s)     : Blinks LED2 at GPIOA Pin 2.
*********************************************************************************************************
*/

static void AppTaskBuzzer(void *p_arg)
{
    (void)p_arg;
    
    while (1) {
        GPIO_SetBits(BLED_GPIO_Port,BLED_Pin);
        OSTimeDlyHMSM(0, 0,0,300);
        GPIO_ResetBits(BLED_GPIO_Port,BLED_Pin);
        OSTimeDlyHMSM(0, 0,0,300);
    }
}
