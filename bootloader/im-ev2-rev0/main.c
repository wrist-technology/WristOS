/**
 * \file main.c
 * Main code
 * 
 * Bluetooth wrist watch image r0.1a
 * 
 * Author: Wrist Technology Ltd  
 *  
 * 
 */
  
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "hardware_conf.h"
#include "firmware_conf.h"
//#include "board_lowlevel.h"
#include <utils/macros.h>

#include <debug/trace.h>
//#include <utils/interrupt_utils.h>
#include <utils/time.h>
#include <peripherals/serial.h>
#include <utils/delay.h>
#include <peripherals/pmc/pmc.h>
#include <peripherals/spi.h>
#include <utils/rprintf.h>
#include <screen/screen.h>
#include <screen/font.h>
#include <utils/fastfixmath.h>

#include <peripherals/i2c.h>
#include <peripherals/isl6295.h>

#include "main.h"

//test:
//#include <peripherals/serial.h>
#include <utils/rscanf.h>



volatile AT91PS_PIO  pPIOA = AT91C_BASE_PIOA;     // PIO controller
volatile AT91PS_PIO  pPIOB = AT91C_BASE_PIOB;     // PIO controller

//scr_buf_t sram_screen[10];
scr_buf_t * scrbuf=NULL;
//scr_bitmapbuf_t * bitmap[(20*20)];



/**
 * The main function.
 * 
 * \return Zero
 * 
 */
int main (void) 
{
    int i,x,y,col;
    uint8_t i2c_r, i2c_byte, newsec, sec, min, hr;
    char s[50];
    char op;
    uint8_t addr;
    //pPIOA = AT91C_BASE_PIOA;
    //pPIOB = AT91C_BASE_PIOB;
    initAIC();
    timer_init(); //re-enable ints
    delayms(100);    
        
    dbg_usart_init();
    
    TRACE_ALL("Ints enabled.");
    
    
    TRACE_ALL("Image loaded.");
    *AT91C_PMC_PCER =   (1 << AT91C_ID_PIOA) |  // Enable Clock for PIO
                        (1 << AT91C_ID_IRQ0);  // Enable Clock for IRQ0
                        
    
    
    i=0;
    col = 0xffffff;
    //scrWriteRect(0,0,159,127,0);
    
    while (1) { delayms(1000); TRACE_ALL("%d \n\r*",timer_get_timer()); };
    
    //scrWriteRect(10,10,70,40,0xffffff);
    
    //TWI TEST
    pPMC->PMC_PCER |= 0x200 | 0x20 | 0x08; //TWI&SPI enable
    pPIOA->PIO_ASR = 0x00000018;
    pPIOA->PIO_MDER = 0x00000018;  //open drain
    pPIOA->PIO_OWDR = 0x00000018;

    //i2cMasterConf(0xff, 1, 0, 0); //1byte addr write
    //i2cMultipleWriteByteInit();

    //for (i=0;i<16;i++)
    //{
     // i2cMultipleWriteByte(i+0x20);
   // }
    //i2cMultipleWriteEnd();
    /*
   i2cMasterConf(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_OPmode<<10)|(I2CGG_REG_OPmode), I2CMASTER_WRITE);
	 i2cWriteByte(I2CGG_POR_BITMASK);
	 
	 i2cMasterConf(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_GPIOctrl<<10)|(I2CGG_REG_GPIOctrl), I2CMASTER_WRITE);
	 i2cWriteByte(0x80);
	 
   i2cMasterConf(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_OPmode<<10)|(I2CGG_REG_OPmode), I2CMASTER_WRITE);
	 i2cWriteByte(I2CGG_SHELF_BITMASK);
	 */
	  (void) scrInit();
	  fontSetCharPos(0,7);    
    fontColor = SCR_COLOR_RED;
    fontBgColor = 0;
    
    //TRACE_SCR("Image Test");
    
    //setup RTC to 0:0:0
    // 
    
    i2cMasterConf((0xd0>>1), 1, 0x1, 0); //1byte addr write
    i2cWriteByte(0);
    i2cMasterConf((0xd0>>1), 1, 0x2, 0); //1byte addr write
    i2cWriteByte(0);
    i2cMasterConf((0xd0>>1), 1, 0x3, 0); //1byte addr write
    i2cWriteByte(0);
    //i2cMasterConf((0xd0>>1), 1, 0x4, 0); //1byte addr write
    //i2cWriteByte(0);
    
    //and setup the alarms:
    i2cMasterConf((0xd0>>1), 1, 0xA, 0); //1byte addr write
    i2cWriteByte(0x00);
    i2cMasterConf((0xd0>>1), 1, 0xB, 0); //1byte addr write
    i2cWriteByte(0xC0);
    i2cMasterConf((0xd0>>1), 1, 0xC, 0); //1byte addr write
    i2cWriteByte(0x0);
    i2cMasterConf((0xd0>>1), 1, 0xD, 0); //1byte addr write minutes
    i2cWriteByte(1);
    i2cMasterConf((0xd0>>1), 1, 0xE, 0); //1byte addr write
    i2cWriteByte(20);
    i2cMasterConf((0xd0>>1), 1, 0xf, 1); //force addr in RTC to 0 not to 0xF
    (void)i2cReadByte();
    
    
    //request off
    TRACE_ALL("RTC and alarms setup. Requested. off and setup PONT.\n\r");
    i2c_r=0;
    
    //PCKEY must be written first
    
    i2cMasterConf((0xfe>>1), 1, 0x85, 0); //1byte addr read
    i2cWriteByte(0xc5);
    i2cMasterConf((0xfe>>1), 1, 0x84, 0); //1byte addr read
    //i2cWriteByte(0x09);      
    i2cWriteByte(0x01);
    
    TRACE_ALL("waiting on PONT.\n\r");
          
    while (1)
    {
      TRACE_ALL("*");
      delay(10);   
    }  
}


/**
 * IO initialization
 * 
 * Initialization for IO pins and ports and internal hardware.
 * 
 */
#if 0
void init_lowlevel_io(void)
{
    volatile AT91PS_SMC2	pSMC = AT91C_BASE_SMC;
    volatile AT91PS_EBI  pEBI = AT91C_BASE_EBI;
    volatile AT91PS_UDP  pUDP = AT91C_BASE_UDP;
    
    // enable reset-button
    //*AT91C_RSTC_RMR = ( 0xA5000000 | AT91C_RSTC_URSTEN );
    
    *AT91C_PMC_PCER =   (1 << AT91C_ID_PIOA) |  // Enable Clock for PIO
                        (1 << AT91C_ID_IRQ0);  // Enable Clock for IRQ0

#if 1
    // set the POWER_ON I/O bit 
    SET(pPIOA->PIO_PER, PIN_POWER_ON);  // Enable PIO pin
    SET(pPIOA->PIO_OER, PIN_POWER_ON);  // Enable output
    SET(pPIOA->PIO_SODR, PIN_POWER_ON);  // Set Low to enable power
    
    SET(pPIOA->PIO_PER, PIN_LED);  // Enable PIO pin
    SET(pPIOA->PIO_OER, PIN_LED);  // Enable output
    SET(pPIOA->PIO_SODR, PIN_LED);  // Set Low to enable power
#else
    #warning Enable Power Pin!
#endif


    // disable USB pull-up 
    //SET(pPIO->PIO_PER, PIN_USB_PULLUP);   // Enable PIO pin
    //SET(pPIO->PIO_OER, PIN_USB_PULLUP);   // Enable outputs
    //SET(pPIO->PIO_SODR, PIN_USB_PULLUP);  // Set high

    // configure USB detection pin
    SET(pPIOB->PIO_PER, PIN_USB_DETECT);     // enable PIO pin
    SET(pPIOB->PIO_ODR, PIN_USB_DETECT);     // set pin as input
    SET(pPIOB->PIO_PPUER, PIN_USB_DETECT);   // enable pull-up

    // Enables the 48MHz USB clock UDPCK and System Peripheral USB Clock
    // Required to write to UDP_TXVC
    SET(pPMC->PMC_SCER, AT91C_PMC_UDP);
    SET(pPMC->PMC_PCER, (1 << AT91C_ID_UDP));
    
    SET(pUDP->UDP_TXVC, AT91C_UDP_TXVDIS);  // disable UDP tranceiver
                                            // because enabled by default after reset
    
    // Disables the 48MHz USB clock UDPCK and System Peripheral USB Clock
    // Save power
    //SET(pPMC->PMC_SCDR, AT91C_PMC_UDP);
    //SET(pPMC->PMC_PCDR, (1 << AT91C_ID_UDP));
    
    //extmem
    
    pEBI->EBI_CSA = 0x0;    
    pSMC->SMC2_CSR[0] = 0x0000B081;
    
}   // end of void init_lowlevel_io(void)
#endif