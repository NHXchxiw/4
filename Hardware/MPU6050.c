#include "MPU6050.h"
#include "MPU6050Rag.h"
#include "delay.h"

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    (void)MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, RegAddress, Data);
}

uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
    uint8_t value = 0u;
    (void)MYI2C_ReadRegs(MPU6050_ADDRESS_7BIT, RegAddress, &value, 1u);
    return value;
}

uint8_t MPU6050_InitChecked(void)
{
    uint8_t id, config[4];
    MYI2C_Init();
    Delay_ms(100u);
    if (!MYI2C_ReadRegs(MPU6050_ADDRESS_7BIT, MPU6050_WHO_AM_I, &id, 1u)
        || (id & 0x7Eu) != 0x68u) return 0u;
    if (!MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_PWR_MGMT_1, 0x80u)) return 0u;
    Delay_ms(100u);
    if (!MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_PWR_MGMT_1, 0x01u)) return 0u;
    Delay_ms(10u);
    /* 1 kHz/(0+1)=1 kHz, DLPF ~42 Hz, gyro +/-2000 dps, accel +/-4 g.
     * This configures the sensor; the task must still read it every 1 ms. */
    if (!MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_PWR_MGMT_2, 0x00u)
        || !MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_SMPLRT_DIV, MPU6050_SMPLRT_DIV_VALUE)
        || !MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_CONFIG, 0x03u)
        || !MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_GYRO_CONFIG, 0x18u)
        || !MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_ACCEL_CONFIG, 0x08u)
        || !MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_USER_CTRL, 0x00u)
        || !MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_INT_PIN_CFG, 0x02u)
        || !MYI2C_WriteReg(MPU6050_ADDRESS_7BIT, MPU6050_INT_ENABLE, 0x00u)) return 0u;
    /* Bypass exposes the GY-86 HMC5883L on the same MCU I2C bus. */
    if (!MYI2C_ReadRegs(MPU6050_ADDRESS_7BIT, MPU6050_SMPLRT_DIV, config, 4u)) return 0u;
    return config[0] == MPU6050_SMPLRT_DIV_VALUE && config[1] == 0x03u
        && config[2] == 0x18u && config[3] == 0x08u;
}

void MPU6050_Init(void)
{
    (void)MPU6050_InitChecked();
}

static int16_t MPU6050_Decode(const uint8_t *p)
{
    return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}

uint8_t MPU6050_ReadRaw(MPU6050_RawData *data)
{
    uint8_t buf[14];
    if (data == 0 || !MYI2C_ReadRegs(MPU6050_ADDRESS_7BIT, MPU6050_ACCEL_XOUT_H, buf, 14u)) return 0u;
    data->ax = MPU6050_Decode(&buf[0]);
    data->ay = MPU6050_Decode(&buf[2]);
    data->az = MPU6050_Decode(&buf[4]);
    data->temperature = MPU6050_Decode(&buf[6]);
    data->gx = MPU6050_Decode(&buf[8]);
    data->gy = MPU6050_Decode(&buf[10]);
    data->gz = MPU6050_Decode(&buf[12]);
    return 1u;
}

void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
    MPU6050_RawData data;
    if (MPU6050_ReadRaw(&data))
    {
        *AccX = data.ax; *AccY = data.ay; *AccZ = data.az;
        *GyroX = data.gx; *GyroY = data.gy; *GyroZ = data.gz;
    }
}
