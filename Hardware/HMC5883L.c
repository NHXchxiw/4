#include "HMC5883L.h"
#include "MYI2C.h"

uint8_t HMC5883L_Init(void)
{
    uint8_t id[3], config[3];
    if (!MYI2C_ReadRegs(HMC5883L_ADDRESS_7BIT, 0x0Au, id, 3u)
        || id[0] != 'H' || id[1] != '4' || id[2] != '3') return 0u;
    if (!MYI2C_WriteReg(HMC5883L_ADDRESS_7BIT, 0x00u, 0x78u)
        || !MYI2C_WriteReg(HMC5883L_ADDRESS_7BIT, 0x01u, 0x20u)
        || !MYI2C_WriteReg(HMC5883L_ADDRESS_7BIT, 0x02u, 0x00u)) return 0u;
    if (!MYI2C_ReadRegs(HMC5883L_ADDRESS_7BIT, 0x00u, config, 3u)) return 0u;
    return config[0] == 0x78u && config[1] == 0x20u && (config[2] & 3u) == 0u;
}

int8_t HMC5883L_ReadRaw(HMC5883L_RawData *data)
{
    uint8_t status, buf[6];
    HMC5883L_RawData sample;
    if (data == 0 || !MYI2C_ReadRegs(HMC5883L_ADDRESS_7BIT, 0x09u, &status, 1u)) return -1;
    /* Reading all six bytes also releases the LOCK bit. */
    if ((status & 3u) == 0u) return 0;
    if (!MYI2C_ReadRegs(HMC5883L_ADDRESS_7BIT, 0x03u, buf, 6u)) return -1;
    /* HMC register order is X, Z, Y (not X, Y, Z). */
    sample.x = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    sample.z = (int16_t)(((uint16_t)buf[2] << 8) | buf[3]);
    sample.y = (int16_t)(((uint16_t)buf[4] << 8) | buf[5]);
    if (sample.x == -4096 || sample.y == -4096 || sample.z == -4096) return -1;
    *data = sample;
    return 1;
}
