#ifndef __MYI2C_H
#define __MYI2C_H

#include <stdint.h>

#define MYI2C_CLOCK_HZ 400000u

/* I2C1: PB8=SCL, PB9=SDA. One task owns the bus; 7-bit addresses.
 * Register transaction APIs are unchanged. Raw bit-banging is no longer used. */
void MYI2C_Init(void);
uint8_t MYI2C_WriteReg(uint8_t address, uint8_t reg, uint8_t value);
uint8_t MYI2C_ReadRegs(uint8_t address, uint8_t reg, uint8_t *data, uint8_t length);
uint8_t MYI2C_Recover(void);

#endif
