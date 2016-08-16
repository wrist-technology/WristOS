/* ========================================================================== */
/*                                                                            */
/*   i2c.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#include <stdlib.h>
#include <unistd.h>
//#include <inttypes.h>
#include <hardware_conf.h>
#include <firmware_conf.h>
#include <peripherals/pmc/pmc.h>
#include <debug/trace.h>
#include <irq_param.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "i2c.h"

//temporary test:

#define I2C_IRQ         0
#define I2C_TIMEOUT     10000

#if 1
#define MUTEX_CREATE    xSemaphoreCreateMutex
#define MUTEX_TAKE      xSemaphoreTake
#define MUTEX_GIVE      xSemaphoreGive
#else
#define MUTEX_CREATE    xSemaphoreCreateRecursiveMutex
#define MUTEX_TAKE      xSemaphoreTakeRecursive
#define MUTEX_GIVE      xSemaphoreGiveRecursive
#endif

static char buff[128];
#if I2C_IRQ
xSemaphoreHandle i2c_semaphore;

extern void (i2c_Isr_Wrapper)(void);
#endif

void i2cMasterConf(uint8_t i2c_addr, uint8_t intaddr_size, uint32_t int_addr, uint8_t read)
{
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;
    uint32_t rflag = 0;

    //setup master mode etc...
    //pTWI->TWI_CR = AT91C_TWI_SWRST;

    //pTWI->TWI_CR = AT91C_TWI_MSDIS;
    //read status - just for clearance
    //rflag=pTWI->TWI_SR;

    rflag=0;

    if (read)
        rflag = AT91C_TWI_MREAD; //read/write access

    //pTWI->TWI_CR &= ~(AT91C_TWI_SWRST);

    switch (intaddr_size)
    {
        case 0: { pTWI->TWI_MMR = (AT91C_TWI_IADRSZ_NO | (((uint32_t)i2c_addr)<<16) | rflag) ; break; }  
        case 1: { pTWI->TWI_MMR = (AT91C_TWI_IADRSZ_1_BYTE | (((uint32_t)i2c_addr)<<16) | rflag) ; break; }
        case 2: { pTWI->TWI_MMR = (AT91C_TWI_IADRSZ_2_BYTE | (((uint32_t)i2c_addr)<<16) | rflag) ; break; }
        case 3: { pTWI->TWI_MMR = (AT91C_TWI_IADRSZ_3_BYTE | (((uint32_t)i2c_addr)<<16) | rflag) ;break; }
    }
    pTWI->TWI_IADR = int_addr;
    //pTWI->TWI_CWGR = 0x048585; //I2C clk cca 9kHz 
    pTWI->TWI_CR = AT91C_TWI_SVDIS | AT91C_TWI_MSEN; //disable slave, enable master
}

//write byte - call i2cMasterConf (phy address, int address, r/w)
int i2cWriteByte(uint8_t data)
{  
    TRACE_I2C("i2cWriteByte()\r\n");
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;
    uint32_t s1,s2;
    //uint32_t tmout = timeval + I2C_TIMEOUT;    // 50ms second timeout
    uint32_t tmout = Timer_tick_count() + I2C_TIMEOUT;    // 50ms second timeout

    //pTWI->TWI_CR = AT91C_TWI_START | AT91C_TWI_STOP;
#if I2C_IRQ
    pTWI->TWI_THR = data;
    pTWI->TWI_IER = AT91C_TWI_TXRDY_MASTER | AT91C_TWI_NACK_MASTER | AT91C_TWI_OVRE | AT91C_TWI_ARBLST_MULTI_MASTER;
    if (xSemaphoreTake(i2c_semaphore, -1) != pdTRUE) {
        TRACE_ERROR("xSemaphoreTake err\r\n");
        return -1;
    }
    s1=pTWI->TWI_SR & AT91C_TWI_TXRDY_MASTER | AT91C_TWI_NACK_MASTER | AT91C_TWI_OVRE | AT91C_TWI_ARBLST_MULTI_MASTER;
    if (!(s1 & AT91C_TWI_TXRDY_MASTER)) {
        sprintf(buff, "i2cWB1 %x", s1);
        panic(buff);
        return -1;
    }
#else
    pTWI->TWI_THR = data;
    //while (!((s1=pTWI->TWI_SR)&AT91C_TWI_TXRDY_MASTER)) if ((s1&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(timeval > tmout)) return;
    while (!((s1=pTWI->TWI_SR)&AT91C_TWI_TXRDY_MASTER)) {
        if ((s1&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(Timer_tick_count() > tmout)) {
            panic("i2cWriteByte1");
            return -1;
        }
    }
#endif
    //crc:
    //pTWI->TWI_THR = 0xCA;
#if _I2C_IRQ
    pTWI->TWI_IER = AT91C_TWI_TXCOMP_MASTER | AT91C_TWI_NACK_MASTER | AT91C_TWI_OVRE | AT91C_TWI_ARBLST_MULTI_MASTER;
    if (xSemaphoreTake(i2c_semaphore, -1) != pdTRUE) {
        TRACE_ERROR("xSemaphoreTake err\r\n");
        return -1;
    }
    s1=pTWI->TWI_SR & AT91C_TWI_TXCOMP_MASTER | AT91C_TWI_NACK_MASTER | AT91C_TWI_OVRE | AT91C_TWI_ARBLST_MULTI_MASTER;
    if (!(s1 & AT91C_TWI_TXCOMP_MASTER)) {
        sprintf(buff, "i2cRB2 %x", s1);
        panic(buff);
        return -1;
    }
#else
    //while (!((pTWI->TWI_SR)&AT91C_TWI_TXRDY_MASTER));

    //while (!((s2=pTWI->TWI_SR)&AT91C_TWI_TXCOMP_MASTER)) if ((s2&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(timeval > tmout)) return;
    while (!((s2=pTWI->TWI_SR)&AT91C_TWI_TXCOMP_MASTER)) {
        if ((s2&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(Timer_tick_count() > tmout)) {
            panic("i2cWriteByte2");
            return -1;
        }
    }
#endif
    return 0;
}

void i2cMultipleWriteByteInit(void)
{  
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;
    pTWI->TWI_CR = AT91C_TWI_START | AT91C_TWI_STOP;
}

void i2cMultipleWriteByte(uint8_t data) 
{ 
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;
    pTWI->TWI_THR = data;

    while (!((pTWI->TWI_SR)&AT91C_TWI_TXRDY_MASTER));
    //crc:
    //pTWI->TWI_THR = 0xCA;
    //while (!((pTWI->TWI_SR)&AT91C_TWI_TXRDY_MASTER));    
}

void i2cMultipleWriteEnd(void)
{ 
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI; 
    while (!((pTWI->TWI_SR)&AT91C_TWI_TXCOMP_MASTER));
}

int i2cReadByte(uint8_t *data)
{  
    TRACE_I2C("i2cReadByte()\r\n");
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;
    uint32_t s1,s2;
    uint32_t tmout = Timer_tick_count() + I2C_TIMEOUT;    // 50ms second timeout
    //if (stop) pTWI->TWI_CR = AT91C_TWI_STOP; //else pTWI->TWI_CR &= ~(AT91C_TWI_STOP);
    //if (start) pTWI->TWI_CR = AT91C_TWI_START; //else pTWI->TWI_CR &= ~(AT91C_TWI_START);
#if I2C_IRQ
    pTWI->TWI_IER = AT91C_TWI_RXRDY | AT91C_TWI_NACK_MASTER | AT91C_TWI_OVRE | AT91C_TWI_ARBLST_MULTI_MASTER;
    pTWI->TWI_CR = AT91C_TWI_START | AT91C_TWI_STOP;
    if (xSemaphoreTake(i2c_semaphore, -1) != pdTRUE) {
        TRACE_ERROR("xSemaphoreTake err\r\n");
        return -1;
    }
    s1=pTWI->TWI_SR & AT91C_TWI_RXRDY | AT91C_TWI_NACK_MASTER | AT91C_TWI_OVRE | AT91C_TWI_ARBLST_MULTI_MASTER;
    if (!(s1 & AT91C_TWI_RXRDY)) {
        if (!(s1 & AT91C_TWI_NACK_MASTER)) {
            sprintf(buff, "i2cRB1 %x", s1);
            panic(buff);
        }
        return -1;
    }
#else
    pTWI->TWI_CR = AT91C_TWI_START | AT91C_TWI_STOP;
    //pTWI->TWI_THR = data;
    while ((!((s1=pTWI->TWI_SR)&AT91C_TWI_RXRDY))) {
        //if ((s1&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(Timer_tick_count() > tmout)) {
        if (s1&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER)) {
            if (!(s1 & AT91C_TWI_NACK_MASTER)) {
                sprintf(buff, "i2cRB1 %x", s1);
                panic(buff);
            }
            return -1;
        }
        if (Timer_tick_count() > tmout) {
            panic("i2cReadByte 1");
            return -1;
        }
    }
#endif
    //if (!to) TRACE_ALL("I2C error:TWI_RXRDY");

    *data = pTWI->TWI_RHR;
#if _I2C_IRQ
    pTWI->TWI_IER = AT91C_TWI_TXCOMP_MASTER | AT91C_TWI_NACK_MASTER | AT91C_TWI_OVRE | AT91C_TWI_ARBLST_MULTI_MASTER;
    if (xSemaphoreTake(i2c_semaphore, -1) != pdTRUE) {
        TRACE_ERROR("xSemaphoreTake err\r\n");
        return -1;
    }
    s1=pTWI->TWI_SR &  AT91C_TWI_TXCOMP_MASTER | AT91C_TWI_NACK_MASTER | AT91C_TWI_OVRE | AT91C_TWI_ARBLST_MULTI_MASTER;
    if (!(s1 & AT91C_TWI_TXCOMP_MASTER)) {
        sprintf(buff, "i2cRB2 %x", s1);
        panic(buff);
        return -1;
    }
#else
    //while ((!((s2=pTWI->TWI_SR)&AT91C_TWI_TXCOMP_MASTER)))  if ((s2&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(timeval > tmout)) return 0;
    while ((!((s2=pTWI->TWI_SR)&AT91C_TWI_TXCOMP_MASTER))) {
        //if ((s2&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(Timer_tick_count() > tmout)) {
        if (s2&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER)) {
            sprintf(buff, "i2cRB2 %x", s1);
            panic(buff);
            return -1;
        }
        if (Timer_tick_count() > tmout) {
            panic("i2cReadByte 2");
            return -1;
        }
    }
#endif
    //if (!to) TRACE_ALL("I2C error:TWI_TXCOMP_MASTER"); 
    return 0;  
}

void i2cMultipleReadByteStart(void)
{
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;
    uint32_t s1,s2;
    //uint32_t tmout = timeval + I2C_TIMEOUT;    // 50ms second timeout
    uint32_t tmout = Timer_tick_count() + I2C_TIMEOUT;    // 50ms second timeout

    pTWI->TWI_CR = AT91C_TWI_START;

    //pTWI->TWI_THR = data;
    //while (!((pTWI->TWI_SR)&AT91C_TWI_RXRDY));  
}


uint8_t i2cMultipleReadByteRead(void)
{
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;
    uint8_t rec;
    uint32_t s1,s2;
    //uint32_t tmout = timeval + I2C_TIMEOUT;    // 50ms second timeout
    uint32_t tmout = Timer_tick_count() + I2C_TIMEOUT;    // 50ms second timeout
    //while ((!((s1=pTWI->TWI_SR)&AT91C_TWI_RXRDY))) if ((s1&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(timeval > tmout)) return 0;
    while ((!((s1=pTWI->TWI_SR)&AT91C_TWI_RXRDY))) {
        if ((s1&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(Timer_tick_count() > tmout)) {
            panic("i2cMultipleReadByteRead");
            return 0;
        }
    }
    //if (!to) TRACE_ALL("I2C error:TWI_RXRDY");
    rec = pTWI->TWI_RHR;
    return rec;
}

uint8_t i2cMultipleReadByteEnd(void)
{
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;
    //uint32_t tmout = timeval + I2C_TIMEOUT;    // 50ms second timeout
    uint32_t tmout = Timer_tick_count() + I2C_TIMEOUT;    // 50ms second timeout
    uint32_t s1,s2;
    pTWI->TWI_CR = AT91C_TWI_STOP;
    uint8_t rec=0;
    //to=500000;
    //while ((!((s1=pTWI->TWI_SR)&AT91C_TWI_RXRDY))) if ((s1&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(timeval > tmout)) return 0;
    while ((!((s1=pTWI->TWI_SR)&AT91C_TWI_RXRDY))) {
        if ((s1&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(Timer_tick_count() > tmout)) {
            panic("i2cMultipleReadByteEnd");
            return 0;
        }
    }
    //if (!to) TRACE_ALL("I2C error:TWI_RXRDY");
    rec = pTWI->TWI_RHR;
    //while ((!((s2=pTWI->TWI_SR)&AT91C_TWI_TXCOMP_MASTER)))  if ((s2&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(timeval > tmout)) return 0;
    while ((!((s2=pTWI->TWI_SR)&AT91C_TWI_TXCOMP_MASTER))) {
        if ((s2&(AT91C_TWI_NACK_MASTER|AT91C_TWI_OVRE|AT91C_TWI_ARBLST_MULTI_MASTER))||(Timer_tick_count() > tmout)) {
            panic("i2cMultipleReadByteEnd 2");
            return 0;
        }
    }
    return rec;
}


// MV

// TODO: add mutex
static unsigned int i2c_open_count = 0;
static xSemaphoreHandle i2c_mutex;


int i2c_init() {
    volatile AT91PS_TWI pTWI = AT91C_BASE_TWI;

    pTWI->TWI_CR = AT91C_TWI_SWRST;
    pTWI->TWI_CR &= ~(AT91C_TWI_SWRST);
    uint32_t s;
    s = pTWI->TWI_SR;
    TRACE_I2C("TWI_SR %x\r\n", s);
    s = pTWI->TWI_SR;
    TRACE_I2C("TWI_SR %x\r\n", s);

    pTWI->TWI_CWGR = 0x048585; //I2C clk cca 9kHz 

    //i2c_mutex = xSemaphoreCreateMutex();
    i2c_mutex = MUTEX_CREATE();
    if(i2c_mutex == NULL) {
        panic("i2c_init");
    }

#if I2C_IRQ
    vSemaphoreCreateBinary(i2c_semaphore);
    xSemaphoreTake(i2c_semaphore, -1);

    // Initialize the interrupts
    pTWI->TWI_IDR = 0xffffffff;

    unsigned int mask = 0x1 << AT91C_ID_TWI;

    // Disable the interrupt controller & register our interrupt handler
    AT91C_BASE_AIC->AIC_IDCR = mask ;
    AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_TWI ] = (unsigned int)i2c_Isr_Wrapper;
    AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_TWI ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | IRQ_I2C_PRI;
    AT91C_BASE_AIC->AIC_ICCR = mask;
    AT91C_BASE_AIC->AIC_IECR = mask;
#endif

    return 0;
}

void i2c_deinit() {
    vQueueDelete(i2c_mutex);
#if I2C_IRQ
    vQueueDelete(i2c_semaphore);
#endif
}

int i2c_open() {
    MUTEX_TAKE(i2c_mutex, -1); 
    if(!i2c_open_count++) {
        pPMC->PMC_PCER = ( (uint32_t) 1 << AT91C_ID_TWI );
    }
    MUTEX_GIVE(i2c_mutex); 
    return 0;
}

int i2c_close() {
    MUTEX_TAKE(i2c_mutex, -1); 
    if(i2c_open_count && --i2c_open_count == 0) {
        pPMC->PMC_PCDR = ( (uint32_t) 1 << AT91C_ID_TWI );
    }
    MUTEX_GIVE(i2c_mutex); 
    return 0;
}

int i2c_lock() {
    MUTEX_TAKE(i2c_mutex, -1); 
}

int i2c_unlock() {
    MUTEX_GIVE(i2c_mutex); 
}

int i2cMasterWrite(uint8_t i2c_addr, uint8_t intaddr_size, uint32_t int_addr, uint8_t data) {
    MUTEX_TAKE(i2c_mutex, -1); 
    pm_lock();
    i2cMasterConf(i2c_addr, intaddr_size, int_addr, I2CMASTER_WRITE);
    int rc = i2cWriteByte(data);
    pm_unlock();
    MUTEX_GIVE(i2c_mutex); 
    return rc;
}

int i2cMasterRead(uint8_t i2c_addr, uint8_t intaddr_size, uint32_t int_addr, uint8_t *data) {
    MUTEX_TAKE(i2c_mutex, -1); 
    pm_lock();
    i2cMasterConf(i2c_addr, intaddr_size, int_addr, I2CMASTER_READ);
    int rc = i2cReadByte(data);
    pm_unlock();
    MUTEX_GIVE(i2c_mutex); 
    return rc;
}
