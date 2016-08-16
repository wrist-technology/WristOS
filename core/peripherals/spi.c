/**
 * \file spi.c
 * SPI interface handler code
 * 
 * Code for SPI interface configuration and handling
 * 
 * AT91SAM7S-128 USB Mass Storage Device with SD Card by Michael Wolf\n
 * Copyright (C) 2008 Michael Wolf\n\n
 * 
 * This program is free software: you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation, either version 3 of the License, or\n
 * any later version.\n\n
 *
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 * GNU General Public License for more details.\n\n
 *
 * You should have received a copy of the GNU General Public License\n
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 *
 */

#include "hardware_conf.h"
#include "firmware_conf.h"
#include <utils/interrupt_utils.h>
#include <irq_param.h>
#include <debug/trace.h>
#include <peripherals/pmc/pmc.h>
#include "rtos.h"
#include "spi.h"


//AT91PS_SPI  pSPI = AT91C_BASE_SPI;      // SPI controller

SPI pSPI = SPI_BASE;
SPI_PIO pSPI_PIO = SPI_PIO_BASE;  

xSemaphoreHandle spi_semaphore;
xSemaphoreHandle spi_mutex;

static spi_scbr = (0x4 << 8);

extern void (SPIIsr_Wrapper)(void);

/**
 * Initialize SPI
 *
 * Initialize all SPI channels and set default speeds
 *
*/
void spi_init(void)
{
    //vSemaphoreCreateBinary(spi_mutex);
    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL) {
        panic("spi_init");
    }

    vSemaphoreCreateBinary(spi_semaphore);
    xSemaphoreTake(spi_semaphore, -1);

    // disable PIO from controlling MOSI, MISO, SCK (=hand over to SPI)
    pSPI_PIO->PIO_PDR = SPI_PIO_MISO | SPI_PIO_MOSI | SPI_PIO_SPCK | SPI_PIO_NPCS;
    
    pSPI_PIO->PIO_OER = SPI_PIO_MOSI | SPI_PIO_SPCK | SPI_PIO_NPCS;
    pSPI_PIO->PIO_ODR = SPI_PIO_MISO;
    // set pin-functions in PIO Controller
    pSPI_PIO->PIO_ASR = SPI_PIO_MISO | SPI_PIO_MOSI | SPI_PIO_SPCK | SPI_PIO_NPCS;


    // enable peripheral clock for SPI ( PID Bit 5 )
    pPMC->PMC_PCER = ( (uint32_t) 1 << AT91C_ID_SPI ); // n.b. IDs are just bit-numbers

    // SPI enable and reset
    pSPI->SPI_CR = AT91C_SPI_SPIEN | AT91C_SPI_SWRST;

    // SPI mode: master, fixed periph. sel., FDIV=0, fault detection disabled
    // with FDIV=0, spi clock = MCK / value in SCBR
    //pSPI->SPI_MR  = AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED | AT91C_SPI_MODFDIS;
    pSPI->SPI_MR  = AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED | AT91C_SPI_MODFDIS | ( 20 << 24 );

    //MV channel 1: accelerometer at -cs0, 16bits/transfer
    //pSPI->SPI_CSR[0] = AT91C_SPI_CPOL  | AT91C_SPI_BITS_16  | (32<<8) | (4<<16) | (1<<24);
    pSPI->SPI_CSR[0] = AT91C_SPI_CPOL  | AT91C_SPI_BITS_8  | (32<<8) | (4<<16) | (1<<24) | AT91C_SPI_CSAAT;

    // channel 2 is PA31, SD-Card
    //pSPI->SPI_CSR[1] = 0x00000400 | AT91C_SPI_NCPHA | AT91C_SPI_CSAAT | AT91C_SPI_BITS_8;
    //OK pSPI->SPI_CSR[1] = (0x04 << 24 /*DLYBCT*/ ) | (0x40 << 16 /*DLYBS*/) | spi_scbr | AT91C_SPI_NCPHA | AT91C_SPI_CSAAT | AT91C_SPI_BITS_8;
    pSPI->SPI_CSR[1] = (0x01 << 24 /*DLYBCT*/ ) | (0x0 << 16 /*DLYBS*/) | spi_scbr | AT91C_SPI_NCPHA | AT91C_SPI_CSAAT | AT91C_SPI_BITS_8;


    // Initialize the interrupts
    pSPI->SPI_IDR = 0xffffffff;

    unsigned int mask = 0x1 << AT91C_ID_SPI;

    // Disable the interrupt controller & register our interrupt handler
    AT91C_BASE_AIC->AIC_IDCR = mask ;
    AT91C_BASE_AIC->AIC_SVR[ AT91C_ID_SPI ] = (unsigned int)SPIIsr_Wrapper;
    AT91C_BASE_AIC->AIC_SMR[ AT91C_ID_SPI ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | IRQ_SPI_PRI;
    AT91C_BASE_AIC->AIC_ICCR = mask ;
    AT91C_BASE_AIC->AIC_IECR = mask;

    // enable SPI
    pSPI->SPI_CR = AT91C_SPI_SPIEN;
}

void spi_close() {
    vQueueDelete(spi_mutex);
    vQueueDelete(spi_semaphore);
}

void spi_lock() {
    //TRACE_INFO(">>>spi_lock(%x)\r\n", xTaskGetCurrentTaskHandle());
    xSemaphoreTake(spi_mutex, -1);
/*
    if(xSemaphoreTake(spi_mutex, 1000 / portTICK_RATE_MS) != pdTRUE) {
        TRACE_ERROR("spi_lock timeout\r\n");
        panic("spi_lock");
    }
*/
    //TRACE_INFO("<<<spi_lock(%x)\r\n", xTaskGetCurrentTaskHandle());
}

void spi_unlock() {
    //TRACE_INFO(">>>spi_unlock(%x)\r\n", xTaskGetCurrentTaskHandle());
    xSemaphoreGive(spi_mutex);
}

int spi_set_clock(uint8_t channel, uint32_t clock) {

    uint8_t div = MCK / clock;
    if (div * clock < MCK) {
        div++;
    }
    TRACE_INFO("spi_set_clock(%d, %d) %d\r\n", channel, clock, div);

    //div = 4;

    pSPI->SPI_CSR[channel] = (pSPI->SPI_CSR[channel] & ~AT91C_SPI_SCBR) | ((div << 8) & AT91C_SPI_SCBR);
    return 0;
}

int spi_ready () {
    int r;
    if ( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY ) ) // Make sure the unit is at rest before we re-begin
    {
        TRACE_SPI("AT91C_SPI_TXEMPTY\r\n");
        while( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY ) );
        while( !( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF ) ) {
            TRACE_SPI("AT91C_SPI_RDRF\r\n");
            r = AT91C_BASE_SPI->SPI_SR;
        }
        r = AT91C_BASE_SPI->SPI_RDR;
    }

    if ( AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF ) {
        TRACE_SPI("AT91C_SPI_RDRF (2)\r\n");
        r = AT91C_BASE_SPI->SPI_RDR;
    }
    return 0;
}

/**
 * Send and Receive an SPI byte.
 *
 * Transmit a byte over SPI and fetch any incoming bytes.
 * \note Note that the "byte" can be from 8 to 16 bit long!
 * \param dout     Byte to send (8-16 bits)
 * \param last     Flag indicating if the CS should be released after transmission, 1 = release
 * \return         Byte received from SPI
 *
*/
uint16_t spi_byte(uint8_t channel, uint16_t dout, uint8_t last)
{
    uint16_t din;

    //spi_lock();

    int address = ~( 1 << channel );

#if 0
    spi_ready();
#else
    while ( !( pSPI->SPI_SR & AT91C_SPI_TDRE ) ); // wait for channel ready
#endif

    // activate required channel
    //pSPI->SPI_MR  = AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED | AT91C_SPI_MODFDIS;
    pSPI->SPI_MR  = AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED | AT91C_SPI_MODFDIS | ( 20 << 24 );

    //pSPI->SPI_MR  |= 0x00010000;  //NCPS1
    pSPI->SPI_MR  |= ((address << 16) & AT91C_SPI_PCS);

    //pSPI->SPI_CSR[1] = 0x00000400 | AT91C_SPI_NCPHA | AT91C_SPI_CSAAT | AT91C_SPI_BITS_8;
    //X pSPI->SPI_CSR[1] = 0x00000000 | spi_scbr | AT91C_SPI_NCPHA | AT91C_SPI_CSAAT | AT91C_SPI_BITS_8;
    
    pSPI->SPI_TDR = dout;

    while ( !( pSPI->SPI_SR & AT91C_SPI_RDRF ) );   // wait for incoming data

    din = pSPI->SPI_RDR ;                           // get received data

    if (last)
        pSPI->SPI_CR = AT91C_SPI_SPIEN | AT91C_SPI_LASTXFER;

    //spi_unlock();

    return din;
}

uint16_t spi_rw_bytes(uint8_t channel, uint8_t *buff_out, uint8_t *buff_in, uint16_t len, uint8_t last)
{
    TRACE_SPI("spi_rw_bytes(%x, %x, %d, %d)\r\n", buff_in, buff_out, len, last);

    //spi_lock();

    int address = ~( 1 << channel );

#if 0
    spi_ready();
#else
    while ( !( pSPI->SPI_SR & AT91C_SPI_TDRE ) ); // wait for channel ready
    //TRACE_SPI("spi_ready\r\n");
#endif

    //TRACE_SPI("spi ready\r\n");

    // activate required channel
    //pSPI->SPI_MR  = AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED | AT91C_SPI_MODFDIS;
    pSPI->SPI_MR  = AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED | AT91C_SPI_MODFDIS | ( 20 << 24 );

    //pSPI->SPI_MR  |= 0x00010000;  //NCPS1
    pSPI->SPI_MR  |= ((address << 16) & AT91C_SPI_PCS);

    //pSPI->SPI_CSR[1] = 0x00000400 | AT91C_SPI_NCPHA | AT91C_SPI_CSAAT | AT91C_SPI_BITS_8;
    //X pSPI->SPI_CSR[1] = 0x00000000 | spi_scbr | AT91C_SPI_NCPHA | AT91C_SPI_CSAAT | AT91C_SPI_BITS_8;
    

    pSPI->SPI_RPR = (uint32_t)buff_in;
    pSPI->SPI_RCR = (uint32_t)len;
    pSPI->SPI_RNPR = (uint32_t)0;
    pSPI->SPI_RNCR = (uint32_t)0;

    pSPI->SPI_TPR = (uint32_t)buff_out;
    pSPI->SPI_TCR = (uint32_t)len;
    pSPI->SPI_TNPR = (uint32_t)0;
    pSPI->SPI_TNCR = (uint32_t)0;

    // enable interrupt 
    pSPI->SPI_IER = AT91C_SPI_ENDRX; 

    // start DMA
    pSPI->SPI_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;

    //TRACE_SPI("spi wait\r\n");
#if 1
    TRACE_SPI("spi_ready %d %d\r\n", pSPI->SPI_RCR, pSPI->SPI_TCR);
    pm_lock();
    if ( xSemaphoreTake(spi_semaphore, -1) != pdTRUE) {
        TRACE_ERROR("xSemaphoreTake err\r\n");
        pm_unlock();
        //spi_unlock();
        return 0;
    }
    pm_unlock();
#else
    uint32_t tmo = Timer_tick_count() + 1000;
    while ( pSPI->SPI_RCR ) {  // wait for incoming data
        //Task_sleep(500);
        if (Timer_tick_count() > tmo) {
            TRACE_ERROR("SPI %d %x\r\n",  pSPI->SPI_RCR, pSPI->SPI_PTSR);
            tmo = Timer_tick_count() + 1000;
        }
    }
#endif

    //TRACE_SPI("dma %x %x %d\r\n", buff_in, pSPI->SPI_RPR, pSPI->SPI_TCR);

    if (last)
        pSPI->SPI_CR = AT91C_SPI_SPIEN | AT91C_SPI_LASTXFER;

    //spi_unlock();

    TRACE_SPI("<<<spi_rw_bytes\r\n");
    return len;
}
