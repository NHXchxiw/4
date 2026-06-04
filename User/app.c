#include "stm32f4xx.h"
#include "app.h"

// 任务堆栈
#define TASK_START_STK_SIZE  128
#define TASK_PPM_STK_SIZE    128
#define TASK_MOTOR_STK_SIZE  128
#define TASK_LED_STK_SIZE    128

OS_STK TaskStartStk[TASK_START_STK_SIZE];
OS_STK TaskPPMStk[TASK_PPM_STK_SIZE];
OS_STK TaskMotorStk[TASK_MOTOR_STK_SIZE];
OS_STK TaskGY86Stk[TASK_LED_STK_SIZE];

// 任务优先级定义
#define TASK_START_PRIO      5
#define TASK_PPM_PRIO        6      // PPM处理优先级较高，保证实时响应
#define TASK_MOTOR_PRIO      7      // 电机控制优先级适中
#define TASK_GY86_PRIO        8      // LED闪烁优先级最低

// 外部变量声明 - 引用ppm.c中定义的变量
extern volatile uint16_t ppm_channels[8];
extern volatile uint8_t  ppm_frame_ready;

// 内部全局变量
volatile uint16_t g_motor_pulse = 1000;  // 电机脉宽，默认1000us（怠速）



int16_t AccX, AccY, AccZ;
int16_t GyroX, GyroY, GyroZ;

void TaskCreate(void)
{
    OSTaskCreate(TaskStart,
                 (void *)0,
                 &TaskStartStk[TASK_START_STK_SIZE - 1u],
                 APP_CFG_STARTUP_TASK_PRIO);
}


static void TaskStart(void *p_arg)
{
    (void)p_arg;
	
	
	ESC_SetThrottle(1000);
	
    OS_CPU_SysTickInitFreq(SystemCoreClock); 

    // 创建任务
    OSTaskCreate(TaskPPM, (void *)0,
                 &TaskPPMStk[TASK_PPM_STK_SIZE-1],
                 TASK_PPM_PRIO);
    
    OSTaskCreate(TaskMotor, (void *)0,
                 &TaskMotorStk[TASK_MOTOR_STK_SIZE-1],
                 TASK_MOTOR_PRIO);
    
	OSTaskCreate(TaskGY86, (void *)0,
                 &TaskGY86Stk[TASK_MOTOR_STK_SIZE-1],
                 TASK_GY86_PRIO);
	
	//删除自身
    OSTaskDel(OS_PRIO_SELF);
}

static void TaskPPM(void *pdata)
{
    (void)pdata;
	
    #if OS_CRITICAL_METHOD == 3
    OS_CPU_SR cpu_sr;    // 为临界区方法3声明的状态保存变量
	#endif
	
    while(1)
    {
        if(ppm_frame_ready)
        {
            uint16_t throttle;
            
            // μC/OS-II 临界区保护
            OS_ENTER_CRITICAL();
            
            throttle = ppm_channels[2];
            ppm_frame_ready = 0;  // 清零标志
            
            // 退出临界区
            OS_EXIT_CRITICAL();
            
            if(throttle < 1000) throttle = 1000;
            if(throttle > 2000) throttle = 2000;
            
            g_motor_pulse = throttle;
        }
        
        OSTimeDlyHMSM(0, 0, 0, 5);
    }
}

// 电机控制任务 - 输出PWM控制信号
void TaskMotor(void *pdata)
{
    (void)pdata;
    
    while(1)
    {
        // 直接设置电机输出
        ESC_SetThrottle(g_motor_pulse);
        
        // 20ms控制周期，符合50Hz PWM要求
        OSTimeDlyHMSM(0, 0, 0, 20);
    }
}

void TaskGY86(void *pdata){
	
	(void)pdata;
	
	while(1){
		// 读取MPU6050数据
		MPU6050_GetData(&AccX, &AccY, &AccZ, &GyroX, &GyroY, &GyroZ);
		
		// 发送格式化的姿态数据
		BLE_SendMPU6050Data(AccX, AccY, AccZ, GyroX, GyroY, GyroZ);
		OSTimeDlyHMSM(0, 0, 0, 50);  // 延时50毫秒
	}
}

