#ifndef _PID_H
#define _PID_H

#include "stm32f4xx.h"

//以MPU6050x轴方向为机头，y轴方向朝机体左侧，z轴方向朝机体上方
//四个电机分别在飞机从上往下看xy轴的四个象限，第一象限正转，第二反转，第三反转，第四正转

typedef enum
{
    CONTROL_LOCKED = 0,
    CONTROL_ARMED,
    CONTROL_FAULT
} Control_State;


typedef struct {
    float Kp;                 // 比例增益

	float angle_target;
	float rate_limit;
} P_Controller;

typedef struct
{
    float Kp;
    float Ki;
    float Kd;

    float i_term;             // 积分对输出的实际贡献，单位 μs
    float rate_last_measured;   // 上次角速度，单位 °/s
    float d_term;         // 滤波后的角速度微分

	
    float i_limit;            // 积分项限幅，单位 μs
    float output_limit;       // 总输出限幅，单位 μs
    float d_tau;              // D项低通滤波时间常数，单位 s

    float output;             // 便于 Watch 观察
    uint8_t initialized;
} PID_Controller;

extern PID_Controller *roll;
extern PID_Controller *pitch;
extern PID_Controller *yaw;

extern float roll_rate_target=0.0f;
extern float pitch_rate_target=0.0f;
extern float yaw_rate_target=0.0f;

extern float roll_angle_target=0.0f;
extern float pitch_angle_target=0.0f;
extern float yaw_angle_target=0.0f;

extern float roll_angle_kp=0.0f;
extern float pitch_angle_kp=0.0f;
extern float yaw_angle_kp=0.0f;

extern float roll_rate_limit=0.0f;
extern float pitch_rate_limit=0.0f;
extern float yaw_rate_limit=0.0f;



float Limit(float value, float min, float max);
void PID_Init(PID_Controller*PID,float Kp,float Ki,float Kd);
void PID_Reset(PID_Controller *PID);
float Angle_P_Update(float angle_target,float angle_measured,float Kp,float rate_limit);
float Rate_PID_Update(PID_Controller*PID,float rate_target,float rate_measured ,float dt);
void Angle_Rate_Init(void);


#endif
