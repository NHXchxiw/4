#include "stm32f4xx.h"                  // Device header
#include "PID.h"

/* 实际存储PID参数和运行状态的三个对象 */
static PID_Controller roll_pid;
static PID_Controller pitch_pid;
static PID_Controller yaw_pid;

/* 三个指针分别指向对应对象 */
PID_Controller *roll  = &roll_pid;
PID_Controller *pitch = &pitch_pid;
PID_Controller *yaw   = &yaw_pid;

float Limit(float value, float min, float max){
	if(value<min)	value=min;
	if(value>max)	value=max;
	return value;
}


void PID_Init(PID_Controller*PID,float Kp,float Ki,float Kd){
	PID->Kp=Kp;
	PID->Ki=Ki;
	PID->Kd=Kd;
	
	PID->i_term=0;
	PID->i_limit=200;
    PID->rate_last_measured = 0.0f;
    PID->d_term = 0.0f;
    PID->output = 0.0f;
    PID->initialized = 0u;
}

void PID_Reset(PID_Controller *PID)
{
    PID->i_term = 0.0f;
    PID->rate_last_measured = 0.0f;
    PID->d_term = 0.0f;
    PID->output = 0.0f;
    PID->initialized = 0u;
}

float Angle_P_Update(float angle_target,float angle_measured,float Kp,float rate_limit){
	//误差
	float angle_error=angle_target-angle_measured;
	
	float rate_target=Kp*angle_error;
	
	rate_target=Limit(rate_target,-rate_limit,rate_limit);
	return rate_target;
}

float Rate_PID_Update(PID_Controller*PID,float rate_target,float rate_measured ,float dt){
	//误差
	float rate_error=rate_target-rate_measured;
	//P项
	float p_term=PID->Kp*rate_error;
	//i项
	float i_term=PID->i_term+PID->Ki*rate_error*dt;
	i_term=Limit(i_term,-PID->i_limit,PID->i_limit);
	//处理积分饱和，如果积分变化能帮助退出饱和，则允许积分
	if (!((PID->output>PID->output_limit&&rate_error>0.0f)||
		(PID->output<-PID->output_limit&&rate_error<0.0f))){
		PID->i_term = i_term;
	}
	//d项,对测量角速度求导并取负号，为实际角速度快速增大时提供阻尼
	float d_raw=(rate_measured-PID->rate_last_measured)/dt;
	PID->d_term=-PID->Kd*d_raw;
	PID->rate_last_measured = rate_measured;
	
	PID->output=p_term+PID->i_term+PID->d_term;
	
	
	
	return PID->output;
}

void Angle_Rate_Init(void){	
	PID_Init(roll,0,0,0);
	PID_Init(pitch,0,0,0);
	PID_Init(yaw,0,0,0);
}


