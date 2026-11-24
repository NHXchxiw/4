#include "stm32f4xx.h"
#include "app.h"
#include "BSP.h"
#include "USART.h"
#include "attitude_control.h"
#include "ANO_Protocol.h"
#include "PID.h"

#define TASK_START_STK_SIZE  128u
#define TASK_PPM_STK_SIZE    128u
#define TASK_MOTOR_STK_SIZE  128u
#define TASK_GY86_STK_SIZE   768u

OS_STK TaskStartStk[TASK_START_STK_SIZE];
OS_STK TaskPPMStk[TASK_PPM_STK_SIZE];
OS_STK TaskMotorStk[TASK_MOTOR_STK_SIZE];
OS_STK TaskGY86Stk[TASK_GY86_STK_SIZE];

#define TASK_START_PRIO      5u
#define TASK_PPM_PRIO        6u
#define TASK_MOTOR_PRIO      7u
#define TASK_GY86_PRIO       8u

OS_EVENT *ImudataReadySem = (OS_EVENT *)0;

extern volatile uint16_t ppm_channels[8];
extern volatile uint8_t ppm_frame_ready;
volatile uint16_t g_motor_pulse = 1000u;
float motor[4];
float throttle=500;
Attitude_Data sample;



static uint32_t Attitude_NowUs(void)
{
    static uint32_t last_cycles;
    static uint32_t elapsed_us;
    static uint32_t remainder;
    uint32_t cycles_per_us = SystemCoreClock / 1000000u;
    uint32_t current = DWT->CYCCNT;
    uint32_t delta = current - last_cycles;

    last_cycles = current;
    if (cycles_per_us == 0u) cycles_per_us = 1u;
    /* Accumulate deltas before conversion: CYCCNT wraps about every 51 s
     * at 84 MHz. The filter and protocol need continuous microseconds. */
    elapsed_us += delta / cycles_per_us;
    remainder += delta % cycles_per_us;
    elapsed_us += remainder / cycles_per_us;
    remainder %= cycles_per_us;
    return elapsed_us;
}


void TaskCreate(void)
{
    OSTaskCreate(TaskStart, (void *)0,
                 &TaskStartStk[TASK_START_STK_SIZE - 1u], APP_CFG_STARTUP_TASK_PRIO);
}

void TaskStart(void *p_arg)
{
    (void)p_arg;
    BSP_Init();
//    ESC_StopAll(); /* Requires PWM_Init() in BSP_Init(). */
    OS_CPU_SysTickInitFreq(SystemCoreClock);

//    OSTaskCreate(TaskPPM, (void *)0, &TaskPPMStk[TASK_PPM_STK_SIZE - 1u], TASK_PPM_PRIO);
//    OSTaskCreate(TaskMotor, (void *)0, &TaskMotorStk[TASK_MOTOR_STK_SIZE - 1u], TASK_MOTOR_PRIO);
    OSTaskCreate(TaskSensorRead, (void *)0, &TaskGY86Stk[TASK_GY86_STK_SIZE - 1u], TASK_GY86_PRIO);
    OSTaskDel(OS_PRIO_SELF);
}



//void TaskPPM(void *pdata)
//{
//    uint16_t channels[PPM_CHANNEL_COUNT];
//    uint16_t throttle;
//    uint8_t valid_mask;
//    (void)pdata;
//    while (1)
//    {
//        /* Poll even without a new group, so lost inputs expire after 100 ms. */
//        valid_mask = PPM_ReadChannels(channels);
//        throttle = ESC_PULSE_STOP;
//        if (valid_mask == PPM_ALL_CHANNELS_MASK &&
//            throttle_channel >= 1u && throttle_channel <= PPM_CHANNEL_COUNT)
//        {
//            throttle = channels[throttle_channel - 1u];
//            if (throttle < 1000u) throttle = 1000u;
//            if (throttle > 2000u) throttle = 2000u;
//        }
//        g_motor_pulse = throttle;
//        OSTimeDlyHMSM(0, 0, 0, 5);
//    }
//}

void TaskMotor(void *pdata){
    (void)pdata;
	Angle_Rate_Init();
    while (1){

    }
}

/* One task owns the bit-banged I2C bus. The scheduler is locked only during
 * the transaction and filter update; serial output happens after unlocking. */
void TaskSensorRead(void *pdata){
	INT8U err;
	for(;;){
		OSSemPend(ImudataReadySem,ready_timeout,&err);
		if(err!=OS_ERR_NONE){
			continue;
		}

		if(!MPU6050_ReadRawDMA_Start(imu_dma_rx)){
			IMU_DMA_WaitComplete(3u);
			continue;
		}

	}
}

void angle_loop(void){
	roll_rate_target =
			Angle_P_Update(roll_angle_target,
						  sample.angle.roll,
						  roll_angle_kp,
						  roll_rate_limit);
	pitch_rate_target =
			Angle_P_Update(pitch_angle_target,
						  sample.angle.pitch,
						  pitch_angle_kp,
						  pitch_rate_limit);
	yaw_rate_target =
			Angle_P_Update(yaw_angle_target,
						  sample.angle.yaw,
						  yaw_angle_kp,
						  yaw_rate_limit);
}

void rate_loop(void){
	roll->output =
			Rate_PID_Update(roll,
						   roll_rate_target,
						   sample.gyro_dps.x,
						   sample.dt);
	pitch->output =
			Rate_PID_Update(pitch,
						   pitch_rate_target,
						   sample.gyro_dps.y,
						   sample.dt);
	yaw->output =
			Rate_PID_Update(yaw,
						   yaw_rate_target,
						   sample.gyro_dps.z,
						   sample.dt);
}

void mix_and_output(void){
	motor[0] = throttle + roll->output + pitch->output - yaw->output;//第一象限
	motor[1] = throttle - roll->output + pitch->output + yaw->output;//第二象限
	motor[2] = throttle + roll->output - pitch->output + yaw->output;//第三象限
	motor[3] = throttle - roll->output - pitch->output - yaw->output;//第四象限
}
