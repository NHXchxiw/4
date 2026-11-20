#include "stm32f4xx.h"
#include "bluetooth.h"
#include <stdlib.h>
/**
  * @brief  发送单个字节到USART1
  */
//void BLE_SendByte(uint8_t data)
//{
//    // 等待发送缓冲区为空
//    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
//    
//    // 发送数据
//    USART_SendData(USART1, data);
//}
void BLE_SendByte(uint8_t data)
{
    uint32_t timeout = 20000; // 设置一个安全的计数上限
    
    // 等待发送缓冲区为空，同时加入超时计数
    while ((USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) && (timeout > 0))
    {
        timeout--;
    }
    
    // 如果是因为超时退出的，说明硬件串口异常，直接放弃发送，防止卡死整个单片机
    if (timeout > 0)
    {
        USART_SendData(USART1, data);
    }
}
/**
  * @brief  发送字符串到USART1
  */
void BLE_SendString(const char* str)
{
    if (str == NULL) return;
    
    while (*str != '\0')
    {
        BLE_SendByte(*str);
        str++;
    }
}

/**
  * @brief  发送16位有符号整数到USART1
  */
void BLE_SendNumber(int16_t num)
{
    char buffer[10];
    uint8_t i = 0;
    
    // 处理负数
    if (num < 0)
    {
        BLE_SendByte('-');
        num = -num;
    }
    // 处理0
    else if (num == 0)
    {
        BLE_SendByte('0');
        return;
    }
    
    // 分离各位数字
    while (num > 0)
    {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }
    
    // 反转发送
    while (i > 0)
    {
        BLE_SendByte(buffer[--i]);
    }
}

/**
  * @brief  发送浮点数到USART1
  * @param  value: 浮点数值
  * @param  decimals: 小数位数
  */
void BLE_SendFloat(float value, uint8_t decimals)
{
    int16_t integer_part;
    float fractional_part;
    uint8_t negative = 0u;

    if (value < 0.0f) { negative = 1u; value = -value; }
    integer_part = (int16_t)value;
    fractional_part = value - integer_part;
    if (negative) BLE_SendByte('-');
    
    // 发送整数部分
    BLE_SendNumber(integer_part);
    
    // 发送小数点
    BLE_SendByte('.');
    
    // 发送小数部分
    for (uint8_t i = 0; i < decimals; i++)
    {
        fractional_part *= 10;
        int16_t digit = (int16_t)fractional_part;
        BLE_SendByte(digit + '0');
        fractional_part -= digit;
    }
}

void BLE_SendAttitudeData(float roll, float pitch, float yaw,
                          float ax, float ay, float az,
                          float gx, float gy, float gz,
                          int16_t mx, int16_t my, int16_t mz,
                          uint16_t status)
{
    BLE_SendString("ATT,"); BLE_SendFloat(roll, 2u); BLE_SendByte(',');
    BLE_SendFloat(pitch, 2u); BLE_SendByte(','); BLE_SendFloat(yaw, 2u);
    BLE_SendByte(','); BLE_SendFloat(ax, 3u); BLE_SendByte(',');
    BLE_SendFloat(ay, 3u); BLE_SendByte(','); BLE_SendFloat(az, 3u);
    BLE_SendByte(','); BLE_SendFloat(gx, 2u); BLE_SendByte(',');
    BLE_SendFloat(gy, 2u); BLE_SendByte(','); BLE_SendFloat(gz, 2u);
    BLE_SendByte(','); BLE_SendNumber(mx); BLE_SendByte(',');
    BLE_SendNumber(my); BLE_SendByte(','); BLE_SendNumber(mz);
    BLE_SendByte(','); BLE_SendNumber((int16_t)status);
    BLE_SendString("\r\n");
}

/**
  * @brief  格式化发送姿态数据
  */
void BLE_SendMPU6050Data(int16_t AccX, int16_t AccY, int16_t AccZ,
                         int16_t GyroX, int16_t GyroY, int16_t GyroZ)
{
    // 发送加速度计数据
	BLE_SendString("AccX:");
    BLE_SendNumber(AccX);
	BLE_SendByte('\n');
	BLE_SendString("AccY:");
    BLE_SendNumber(AccY);
	BLE_SendByte('\n');
	BLE_SendString("AccZ:");
    BLE_SendNumber(AccZ);
	BLE_SendByte('\n');
    
    // 发送陀螺仪数据
	BLE_SendString("GyroX:");
    BLE_SendNumber(GyroX);
	BLE_SendByte('\n');
	BLE_SendString("GyroY:");
    BLE_SendNumber(GyroY);
	BLE_SendByte('\n');
	BLE_SendString("GyroZ:");
    BLE_SendNumber(GyroZ);
    
    // 发送换行
    BLE_SendByte('\r');
    BLE_SendByte('\n');
}
