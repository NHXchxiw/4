#ifndef _APP_H_
#define _APP_H_
#include "includes.h"

typedef struct
{
    MPU6050_RawData imu;

    uint32_t timestamp_cycles;  // 对应数据就绪时刻
    uint32_t sequence;          // 样本序号
    uint16_t status;            // 读取是否成功等状态
} ImuFrame;

typedef struct
{
    Attitude_Data attitude;     // 包含角度、去零偏角速度等

    uint32_t timestamp_cycles;  // 继承原始样本时间
    uint32_t sequence;          // 继承原始样本序号
} FlightFrame;

/*
*********************************************************************************************************
*                                        FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void TaskCreate(void);
void TaskStart(void *pdata);
void TaskPPM(void *pdata);
void TaskMotor(void *pdata);
void TaskGY86(void *pdata);
void angle_loop(void);
void rate_loop(void);
void mix_and_output(void);


#endif /* _APP_H_ */
