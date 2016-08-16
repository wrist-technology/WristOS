#include "accel.h"
#include "debug/trace.h"
#include "spi.h"
#include <hardware_conf.h>

void accel_reg_write8(uint8_t reg, uint8_t value) {
    spi_byte(0, ACCEL_REG_WRITE | (reg & ACCEL_REG_MASK), 0);
    spi_byte(0, value, 1);
}

uint8_t accel_reg_read8(uint8_t reg) {
    spi_byte(0, ACCEL_REG_READ | (reg & ACCEL_REG_MASK), 0);
    return spi_byte(0, 0, 1);
}

void accel_reg_write16(uint8_t reg, uint16_t value) {
#if 1
    spi_byte(0, ACCEL_REG_WRITE | ACCEL_REG_MULTI | (reg & ACCEL_REG_MASK), 0);
    spi_byte(0, value & 0xff, 0);
    spi_byte(0, (value >> 8) & 0xff, 1);
#else
    spi_byte(0, (reg & ACCEL_REG_MASK), 0);
    spi_byte(0, value & 0xff, 1);

    spi_byte(0, ((reg + 1) & ACCEL_REG_MASK), 0);
    spi_byte(0, (value >> 8) & 0xff, 1);
#endif
}

int16_t accel_reg_read16(uint8_t reg) {
    spi_byte(0, ACCEL_REG_READ | ACCEL_REG_MULTI | (reg & ACCEL_REG_MASK), 0);
    uint8_t l = spi_byte(0, 0, 0);
    uint8_t h = spi_byte(0, 0, 1);
    return (h << 8) | l;
}

int accel_start() {
    //switch on:
    spi_lock();

    accel_reg_read8(ACCEL_REG_DD_ACK);

    accel_reg_write8(ACCEL_REG_CTRL_REG1, 0xC7);
    accel_reg_write8(ACCEL_REG_CTRL_REG2, 0x08); // INT
    //accel_reg_write8(ACCEL_REG_CTRL_REG2, 0x04);   // DRDY
    accel_reg_write8(ACCEL_REG_CTRL_REG3, 0x4b);

    // DD threshold (0xffff / 2 = 0x7fff = FS (2g)
    // => 1g = 0x7fff / 2 = 0x3fff (16383)
    accel_reg_write16(ACCEL_REG_DD_THSE_L, 0x2800); // 0.6g
    //accel_reg_write16(ACCEL_REG_DD_THSI_L, 0x2600);
    accel_reg_write16(ACCEL_REG_DD_THSI_L, 0x1300); // 0.3g

    accel_reg_read8(ACCEL_REG_HP_FILTER_RESET);
    //accel_reg_write8(ACCEL_REG_DD_CFG, 0xcf); // X & Y DD
    //accel_reg_write8(ACCEL_REG_DD_CFG, 0xfc); // Y & Z DD
    //accel_reg_write8(ACCEL_REG_DD_CFG, 0xcc); // Y (hi/low) only
    accel_reg_write8(ACCEL_REG_DD_CFG, 0xc4); // Y (low) only

    spi_unlock();
}

int accel_stop() {
    //switch off:
    spi_lock();

    accel_reg_write8(ACCEL_REG_CTRL_REG1, 0x00);

    spi_unlock();
}

static uint8_t out[7] = {ACCEL_REG_READ | ACCEL_REG_MULTI | ACCEL_REG_OUTX_L, 0, 0, 0, 0, 0, 0, 0};
static uint8_t in[7];

int accel_read(int16_t *x, int16_t *y, int16_t *z, bool lock) {
    if (lock) 
        spi_lock();

/*
    while(1) {
        uint8_t s = accel_reg_read8(ACCEL_REG_STATUS_REG);
        if (s & 0x08)
            break;
    }
*/

#if 1 
    *x = accel_reg_read16(ACCEL_REG_OUTX_L);
    *y = accel_reg_read16(ACCEL_REG_OUTY_L);
    *z = accel_reg_read16(ACCEL_REG_OUTZ_L);
#else

    uint8_t l, h;

    spi_rw_bytes(0, out, in, 7, 1);
    l = in[1];
    h = in[2];
    *x = (h << 8) | l;
    l = in[3];
    h = in[4];
    *y = (h << 8) | l;
    l = in[5];
    h = in[2];
    *z = (h << 8) | l;
#endif

    if (lock)
        spi_unlock();

    return 0;
}

/*

void spiConf()
{
  volatile AT91PS_SPI pSPI = AT91C_BASE_SPI;
  pSPI->SPI_CR = AT91C_SPI_SWRST; //reset
  pSPI->SPI_CR = 0;
  pSPI->SPI_CR = AT91C_SPI_SPIEN; //enable
  pSPI->SPI_MR = AT91C_SPI_MSTR | (20 << 24);
  //accelerometer at -cs0, 16bits/transfer
  pSPI->SPI_CSR[0] = AT91C_SPI_CPOL  | AT91C_SPI_BITS_16  | (32<<8) | (4<<16) | (1<<24);
  //microSD - TBD - not tested $$$
  pSPI->SPI_CSR[1] = AT91C_SPI_CPOL  | AT91C_SPI_BITS_8  | (32<<8) | (4<<16) | (1<<24);

}

int16_t spiAccelRead16(uint8_t reg)
{
    volatile AT91PS_SPI pSPI = AT91C_BASE_SPI;
    int16_t rx=0;

    TRACE_ACCEL("spiAccelRead16(%x)\r\n", reg);
#if 1
    spi_ready();
#else
    while (!((pSPI->SPI_SR)&AT91C_SPI_TDRE));
#endif



    pSPI->SPI_MR = AT91C_SPI_MSTR | (20 << 24); //CS0

    pSPI->SPI_TDR = (((reg&0x3F) | SPIACCEL_REGMREAD | SPIACCEL_REGREAD )<<8); //CS0:

    //while (!((pSPI->SPI_SR)&AT91C_SPI_TDRE));
    while ( !( pSPI->SPI_SR & AT91C_SPI_RDRF ) );

    rx = (pSPI->SPI_RDR&0xff00); //low bytes
    //rx = (pSPI->SPI_RDR&0xff00)>>8; //low bytes
    pSPI->SPI_CR = AT91C_SPI_LASTXFER;
    pSPI->SPI_TDR=0;
    //while (!((pSPI->SPI_SR)&AT91C_SPI_TDRE));
    while ( !( pSPI->SPI_SR & AT91C_SPI_RDRF ) );

    //rx |= ((pSPI->SPI_RDR&0xff)<<8); //high bytes
    rx |= ((pSPI->SPI_RDR&0xff)); //high bytes
    //rx |= ((pSPI->SPI_RDR&0xff00)); //high bytes
    //while ( !( pSPI->SPI_SR & AT91C_SPI_RDRF ) );

    return rx;
}

void spiAccelWrite8(uint8_t reg, uint8_t data)
{
    volatile AT91PS_SPI pSPI = AT91C_BASE_SPI;

    TRACE_ACCEL("spiAccelWrite8(%x, %x)\r\n", reg, data);

// MV
#if 1
    spi_ready();
#else
    while (!((pSPI->SPI_SR)&AT91C_SPI_TDRE));
#endif

    pSPI->SPI_MR = AT91C_SPI_MSTR | (20 << 24); //CS0
    pSPI->SPI_CR = AT91C_SPI_LASTXFER;
    pSPI->SPI_TDR = (((reg&0x3F))<<8) | (data); //CS0:

    //while (!((pSPI->SPI_SR)&AT91C_SPI_TDRE));
    while ( !( pSPI->SPI_SR & AT91C_SPI_RDRF ) );
}

int accel_start() {
    //switch on:
    spi_lock();
    spiAccelWrite8(SPIACCEL_CTRL_REG1, 0xC7);
    spiAccelWrite8(SPIACCEL_CTRL_REG2, 0x40);
    spiAccelWrite8(SPIACCEL_CTRL_REG3, 0x00);
    spi_unlock();
    Task_sleep(10);
}

int accel_read(int *x, int *y, int *z) {
    spi_lock();
    *x = spiAccelRead16(SPIACCEL_REG_XL);
    *y = spiAccelRead16(SPIACCEL_REG_YL);
    *z = spiAccelRead16(SPIACCEL_REG_ZL);
    spi_unlock();

    return 0;
}
*/

