#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

#define I2CMASTER_WRITE 0
#define I2CMASTER_READ 1

void i2cMasterConf(uint8_t i2c_addr, uint8_t intaddr_size, uint32_t int_addr, uint8_t read);

int i2cWriteByte(uint8_t data);

int i2cReadByte(uint8_t *data);

void i2cMultipleReadByteStart(void);
uint8_t i2cMultipleReadByteRead(void);
uint8_t i2cMultipleReadByteEnd(void);

void i2cMultipleWriteByteInit(void);
void i2cMultipleWriteByte(uint8_t data);
void i2cMultipleWriteEnd(void);

int i2cMasterWrite(uint8_t i2c_addr, uint8_t intaddr_size, uint32_t int_addr, uint8_t data);
int i2cMasterRead(uint8_t i2c_addr, uint8_t intaddr_size, uint32_t int_addr, uint8_t *data);

#endif // I2C_H_
