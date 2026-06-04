#include "stm32f4xx.h"                  // Device header
#include "MYI2C.h"
#include "MPU6050Rag.h"

#define MPU6050_ADDRESS	0xD0

//指定地址写
void MPU6050_WriteReg(uint8_t RegAddress,uint8_t Data){
	MYI2C_Start();
	MYI2C_SendByte(MPU6050_ADDRESS);//发送从机地址
	MYI2C_ReceiveAck();
	MYI2C_SendByte(RegAddress);
	MYI2C_ReceiveAck();
	MYI2C_SendByte(Data);
	MYI2C_ReceiveAck();
	MYI2C_Stop();
}



uint8_t MPU6050_ReadReg(uint8_t RegAddress){
	uint8_t Data;
	
	MYI2C_Start();
	MYI2C_SendByte(MPU6050_ADDRESS);//发送从机地址
	MYI2C_ReceiveAck();
	MYI2C_SendByte(RegAddress);
	MYI2C_ReceiveAck();
	
	//转入读时序，重新指定读写位
	MYI2C_Start();
	MYI2C_SendByte(MPU6050_ADDRESS | 0x01);//写地址或上0x01,读写位为1
	MYI2C_ReceiveAck();
	Data = MYI2C_ReceiveByte();
	MYI2C_SendAck(1);
	MYI2C_Stop();
	return Data;
}

void MPU6050_Init(void){
	MYI2C_Init();
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1,0x01);
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2,0x00);
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV,0x09);
	MPU6050_WriteReg(MPU6050_CONFIG,0x06);
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG,0x18);
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG,0x18);
}

void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ){
	uint8_t DataH, DataL;								//定义数据高8位和低8位的变量
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);		//读取加速度计X轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);		//读取加速度计X轴的低8位数据
	*AccX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);		//读取加速度计Y轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);		//读取加速度计Y轴的低8位数据
	*AccY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);		//读取加速度计Z轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);		//读取加速度计Z轴的低8位数据
	*AccZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);		//读取陀螺仪X轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);		//读取陀螺仪X轴的低8位数据
	*GyroX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);		//读取陀螺仪Y轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);		//读取陀螺仪Y轴的低8位数据
	*GyroY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);		//读取陀螺仪Z轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);		//读取陀螺仪Z轴的低8位数据
	*GyroZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
}
