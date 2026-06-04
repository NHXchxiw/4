#include "stm32f4xx.h"
#include "ucos_ii.h"
#include "BSP.h"

// 优先级定义 - 关键！
#define  APP_CFG_STARTUP_TASK_PRIO         3u
#define  APP_CFG_BLUETOOTH_TASK_PRIO       4u   // 高优先级
#define  APP_CFG_LED_TASK_PRIO             8u   // 低优先级

// 堆栈大小
#define  APP_STK_SIZE                      256u

// 全局标志
volatile uint8_t g_data_received = 0;  // 0:正常模式 1:快速闪烁 2:长灭

static  OS_STK  AppTaskStartStk[APP_STK_SIZE];
static  OS_STK  AppTaskLEDStk[APP_STK_SIZE];
static  OS_STK  AppTaskBluetoothStk[APP_STK_SIZE];

static  void  AppTaskStart(void *p_arg);
static  void  AppTaskLED(void *p_arg);
static  void  AppTaskBluetooth(void *p_arg);

void AppTaskCreate(void)
{
    OSTaskCreate(AppTaskStart,
                 (void *)0,
                 &AppTaskStartStk[APP_STK_SIZE - 1u],
                 APP_CFG_STARTUP_TASK_PRIO);
}

static void AppTaskStart(void *p_arg)
{
    (void)p_arg;

    OS_CPU_SysTickInitFreq(SystemCoreClock); 
	
    BSP_Init();

    // 创建任务
    OSTaskCreate(AppTaskBluetooth,
                 (void *)0,
                 &AppTaskBluetoothStk[APP_STK_SIZE - 1u],
                 APP_CFG_BLUETOOTH_TASK_PRIO);  // 高优先级
    
    OSTaskCreate(AppTaskLED,
                 (void *)0,
                 &AppTaskLEDStk[APP_STK_SIZE - 1u],
                 APP_CFG_LED_TASK_PRIO);         // 低优先级

    OSTaskDel(OS_PRIO_SELF);
}

// LED任务 - 低优先级（主要观察对象）
static void AppTaskLED(void *p_arg)
{
    (void)p_arg;
    
    while (1) {
        // ===== 状态1：正常慢闪（默认状态） =====
        if(g_data_received == 0) {
            GPIO_SetBits(BLED_GPIO_Port, BLED_Pin);   // 亮
            OSTimeDlyHMSM(0, 0, 0, 500);              // 500ms
            GPIO_ResetBits(BLED_GPIO_Port, BLED_Pin); // 灭
            OSTimeDlyHMSM(0, 0, 0, 500);              // 500ms
        }
        
        else if(g_data_received == 1) {
            GPIO_ResetBits(BLED_GPIO_Port, BLED_Pin);  // 确保熄灭
            OSTimeDlyHMSM(0, 0, 2, 0);                 // 灭2秒
            g_data_received = 0;  // 恢复正常模式
        }
    }
}

// 蓝牙任务 - 高优先级（抢占的发起者）
static void AppTaskBluetooth(void *p_arg)
{
    (void)p_arg;
    
    while (1) {
        // 每5秒模拟收到一次蓝牙数据
        OSTimeDlyHMSM(0, 0, 5, 0);
        
        // 关键！设置标志触发LED任务改变行为
        // 由于本任务优先级高，会立即抢占LED任务
        g_data_received = 1;
        
        // 本任务立即让出CPU，LED任务接着运行并响应标志
        // （这里不做其他操作，体现纯任务切换）
    }
}