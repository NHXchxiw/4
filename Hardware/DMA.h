#ifndef __DMA_H
#define __DMA_H

#include "stm32f4xx.h"
#include "ucos_ii.h"

#define MPU_DMA_RX_SIZE 14u

typedef enum
{
    IMU_DMA_IDLE = 0,

    IMU_DMA_STARTING,    /* CPU正在完成I2C寻址 */
    IMU_DMA_RECEIVING,   /* DMA正在接收 */
    IMU_DMA_DONE,        /* DMA完成，等待任务处理结果 */
    IMU_DMA_ERROR
} IMU_DMA_State;

extern uint8_t imu_dma_rx[MPU_DMA_RX_SIZE];

extern volatile IMU_DMA_State ImuDmaState;

extern OS_EVENT *ImuDmaDoneSem;

/* 使用上一条给出的DMA初始化配置 */
void IMU_DMA_Init(void);

/*
 * 从MPU6050的0x3B开始，启动14字节DMA读取。
 *
 * 返回1：启动成功，数据还不一定接收完成。
 * 返回0：没有成功启动，不能等待本次完成通知。
 *
 * 只能由统一管理I2C1的采集任务调用。
 * buffer至少有14字节，且DMA完成前必须保持有效。
 */
extern uint8_t MPU6050_ReadRawDMA_Start(uint8_t *buffer);

uint8_t IMU_DMA_WaitComplete(uint32_t timeout_ticks);
uint8_t IMU_DMA_Abort(void);

#endif
