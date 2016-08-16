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
    
    dbg_usart_init();
    
    
    TRACE_ALL("Image loaded.");
    
    timer_init(); //re-enable ints
    i=0;
    col = 0xffffff;
    scrWriteRect(0,0,159,127,0);
    
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
    
    i2c_r=0;
    while (1)
    {
      i2c_r++;i2c_r&=0x0f;
      
      i2cMasterConf((0xd0>>1), 1, 0x1, 1); //1byte addr read
      newsec = i2cReadByte()&0x7F;
      newsec = (((newsec&0xf0)>>4)*10) + (newsec&0xf);
      
      if (newsec!=sec)
      {
        sec=newsec;
        
        //read minutes
        i2cMasterConf((0xd0>>1), 1, 0x2, 1); //1byte addr read
        min = i2cReadByte()&0x7F;
        min = (((min&0xf0)>>4)*10) + (min&0xf);
        //read hours
        i2cMasterConf((0xd0>>1), 1, 0x3, 1); //1byte addr read
        hr = i2cReadByte()&0x3F;
        hr = (((hr&0xf0)>>4)*10) + (hr&0xf);
        
        fontSetCharPos(50,57);    
        fontColor = SCR_COLOR_YELLOW;
        TRACE_SCR("%2d:%2d:%2d",hr,min,sec);
        
        
        //i2cMasterConf((0xfe>>1), 1, 0x87, 1); //1byte addr read      
        //i2c_byte = i2cReadByte();
        //TRACE_ALL("0%d\n\r",i2c_byte);  
      }
      
      //i2cMasterConf((0xfe>>1), 1, 0xf0+i2c_r, 0); //1byte addr write
      //i2cWriteByte(i2c_r+0xa0);
      
      
      //i2cMasterConf((0xfe>>1), 1, 0xf0+i2c_r, 1); //1byte addr read
      
      //i2cMasterConf((0x0e>>1), 1, 0x87, 1); //1byte addr read
      //i2cMasterConf((0xd0>>1), 1, 0x1, 1); //1byte addr read
	    
	    //TRACE_ALL("[0x86:%x]\n\r",i2c_byte);

      //for (i=0;i<200000;i++) SET(pPIOA->PIO_SODR, PIN_LED);
      delayms(10);
      //i2c_byte=50;
      
      fontSetCharPos(0,7);
      scanline(s);
      TRACE_SCR("%s\n\r",s);
      //sscanf(s,"%c %x",&op,&addr);
      op=s[0];
      s[0]=' ';
      addr=(uint8_t)atoi(s);
      switch (op)
      {
        case 'r':
        {
          i2cMasterConf((0xfe>>1), 1, addr, 1); //1byte addr read
          i2c_byte = i2cReadByte();
          TRACE_ALL("i2c read:0x%x : %x \n\r",addr, i2c_byte);
          break;
        }
        case 'w':
        {
          TRACE_ALL("enter data to write (decimal):");
          scanline(s);
          i2c_byte = (uint8_t)atoi(s);
          
          if (addr==0x84) //PCKEY must be written first
          {
            i2cMasterConf((0xfe>>1), 1, 0x85, 0); //1byte addr read
            i2cWriteByte(0xc5);
          }
          i2cMasterConf((0xfe>>1), 1, addr, 0); //1byte addr read
          i2cWriteByte(i2c_byte);
                    
          TRACE_ALL("i2c write:0x%x : %x \n\r",addr, i2c_byte);
          break;
        }
        default:
        {
          TRACE_ALL("err.[%c]/%s/ ",op,s);
          break;
        }
      
      }
      //if (TRACE_SCR("%s",);
      //TRACE_SCR("%c",dbg_usart_getchar());
      
      
      
      
      //RTC read:
      //i2cMasterConf((0xd0>>1), 1, 0x1, 1);
      //i2c_r=i2cReadByte();
      
      //TRACE_ALL("0x%x  0x%x \n\r",i2c_byte, i2c_r); 
      
      
      i+=(FFM_UNIT/50);
      if (i>(2*FFM_PI)) { i=0; col^=0xffffff;}//scrWriteRect(0,0,159,127,0); };
      x = (ffm_sin(i)/(FFM_UNIT/50))+79;
      y = (ffm_sin(i+(FFM_PI/2))/(FFM_UNIT/50))+62;
      scrWriteRect(x,y,x+1,y+1,col);
      
      
      
      if (!((pPIOB->PIO_PDSR)&(BUT1_MASK))) scrWriteRect(0,50,5,55,0xffffff); else scrWriteRect(0,50,5,55,0);               
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