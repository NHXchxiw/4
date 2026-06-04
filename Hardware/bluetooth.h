#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "stm32f4xx.h"

// 函数声明
void BLE_SendByte(uint8_t data);
void BLE_SendString(const char* str);
void BLE_SendNumber(int16_t num);
void BLE_SendFloat(float value, uint8_t decimals);
void BLE_SendMPU6050Data(int16_t AccX, int16_t AccY, int16_t AccZ,
                         int16_t GyroX, int16_t GyroY, int16_t GyroZ);

#endif /* __BLUETOOTH_H */
