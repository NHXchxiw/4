#ifndef __HMC5883L_H
#define __HMC5883L_H

#include <stdint.h>

#define HMC5883L_ADDRESS_7BIT 0x1Eu
#define HMC5883L_LSB_PER_GAUSS 1090.0f

typedef struct
{
    int16_t x, y, z;
} HMC5883L_RawData;

/* Init after MPU6050 enables I2C bypass. H43 ID, 8 averages, 75 Hz, +/-1.3 G. */
uint8_t HMC5883L_Init(void);
/* 1 = fresh sample, 0 = not ready, -1 = bus failure or overflow. */
int8_t HMC5883L_ReadRaw(HMC5883L_RawData *data);

#endif
