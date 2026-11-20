#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "stm32f4xx.h"
#include "usart1.h"  /* Same-directory saved Bluetooth port, PB6/PB7. */

// 函数声明
void BLE_SendByte(uint8_t data);
void BLE_SendString(const char* str);
void BLE_SendNumber(int16_t num);
void BLE_SendFloat(float value, uint8_t decimals);
void BLE_SendMPU6050Data(int16_t AccX, int16_t AccY, int16_t AccZ,
                         int16_t GyroX, int16_t GyroY, int16_t GyroZ);
void BLE_SendAttitudeData(float roll, float pitch, float yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz,
                          int16_t mx, int16_t my, int16_t mz,
                          uint16_t status);

#endif /* __BLUETOOTH_H */
