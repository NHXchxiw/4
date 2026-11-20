#ifndef __MPU6050_H
#define __MPU6050_H

#include "MYI2C.h"

#define MPU6050_ADDRESS_7BIT     0x68u /* AD0 low; use 0x69 when high. */
#define MPU6050_SAMPLE_HZ        1000u
/* With DLPF enabled, the gyro output clock is 1 kHz. */
#define MPU6050_SMPLRT_DIV_VALUE (1000u / MPU6050_SAMPLE_HZ - 1u)
#if MPU6050_SAMPLE_HZ == 0u || MPU6050_SAMPLE_HZ > 1000u
#error "MPU6050_SAMPLE_HZ must be in 1..1000 Hz"
#elif (1000u % MPU6050_SAMPLE_HZ) != 0u
#error "MPU6050_SAMPLE_HZ must divide 1000 Hz exactly"
#endif
#define MPU6050_ACCEL_LSB_PER_G  8192.0f /* +/-4 g */
#define MPU6050_GYRO_LSB_PER_DPS 16.4f   /* +/-2000 deg/s */

typedef struct
{
    int16_t ax, ay, az;
    int16_t temperature;
    int16_t gx, gy, gz;
} MPU6050_RawData;

/* Checked APIs return 1 on success, 0 on bus/device failure. */
uint8_t MPU6050_InitChecked(void);
uint8_t MPU6050_ReadRaw(MPU6050_RawData *data);
/* Existing interfaces retained for older callers. */
void MPU6050_Init(void);
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_ReadReg(uint8_t RegAddress);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);
#endif
