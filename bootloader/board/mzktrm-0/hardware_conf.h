/**
 * \file hardware_conf.h
 * Header: Hardware configuration
 *
 * Configuration for all hardware related settings.
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
#ifndef HARDWARE_CONF_H_
#define HARDWARE_CONF_H_

//#include "AT91SAM7SE512.h"
#include "AT91SAM7S128.h"

#define BOARD_V2

#define SCR_COLORDEPTH 8
/**
 * \name Clock constants
 *
*/
//@{
#define EXT_OC          18432000   //!< External oscillator MAINCK
#define MCK             47923200   //!< MCK (PLLRC div by 2)
//@}


// USB pullup
#define USB_PIO    AT91PS_PIO
#define USB_PIO_BASE  AT91C_BASE_PIOA  
#define USB_PIO_PULLUP  AT91C_PIO_PA3

/**
 * \def SPI interface
 * 
 *  
*/

#define SPI        AT91PS_SPI
#define SPI_BASE   AT91C_BASE_SPI
#define SPI_PIO    AT91PS_PIO
#define SPI_PIO_BASE  AT91C_BASE_PIOA  
#define SPI_PIO_MISO  AT91C_PA12_MISO 
#define SPI_PIO_MOSI  AT91C_PA13_MOSI 
#define SPI_PIO_SPCK  AT91C_PA14_SPCK 
#define SPI_PIO_NPCS  AT91C_PA11_NPCS0
#define SPI_PIO_CSRTC  AT91C_PIO_PA8

#define SDCARD_SPI_CHANNEL 0
//actually this is not used (CS manually) but must be diffrent: SD card collision
#define RTC_SPI_CHANNEL 1 

#define SOUND_VOL_CH 0
#define SOUND_FREQ_CH 2

#define PWM_PIO_PWMVOL AT91C_PA0_PWM0
#define PWM_PIO_PWM  AT91C_PA2_PWM2
#define PWM_PIO_BASE  AT91C_BASE_PIOA 

#define SPMEN_PIO AT91C_PIO_PA26

/**
 * \def DBG_USART_BAUDRATE
 * Baudrate for DEBUG USART communication inface and speed
 *
*/
#define DBG_USART              AT91PS_DBGU
#define DBG_USART_BAUDRATE     115200
#define DBG_USART_BASE         AT91C_BASE_DBGU
#define DBG_PIO                AT91PS_PIO
#define DBG_PIO_BASE           AT91C_BASE_PIOA
#define DBG_PIN_RXD            AT91C_PA9_DRXD 
#define DBG_PIN_TXD            AT91C_PA10_DTXD

// Normal mode  // Clock = MCK // 8-bit data // No parity
#define DBG_USART_CONF   (AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK \
                              | AT91C_US_CHRL_8_BITS   \
                              | AT91C_US_PAR_NONE      \
                              | AT91C_US_NBSTOP_1_BIT)      // 1 Stop bit
                              
                              
//////////      
#define MZKT_USART_BAUDRATE 19200
#define MZKT_BAUDRATE_DIV    (MCK/16/MZKT_USART_BAUDRATE)
#define  MZKT_USART_CONFR (AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK \
                              | AT91C_US_CHRL_8_BITS   \
                              | AT91C_US_PAR_EVEN      \
                              | AT91C_US_NBSTOP_1_BIT)      // 1 Stop bit
                              
#define  MZKT_USART_CONFT (AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK \
                              | AT91C_US_CHRL_8_BITS   \
                              | AT91C_US_PAR_EVEN      \
                              | AT91C_US_NBSTOP_15_BIT )      // 1 Stop bit                              
                                          
#define MZKT_PIO_BASE   AT91C_BASE_PIOA                                                  
#define MZKT_PIN_TXD    AT91C_PA6_TXD0
#define MZKT_PIN_RXD    AT91C_PA5_RXD0    
#define MZKT_PIN_TXEN   AT91C_PIO_PA4                        


//LCD module
#define LCD_PIO_BASE     AT91C_BASE_PIOA
#define LCDDB0           AT91C_PIO_PA20
#define LCDDB1           AT91C_PIO_PA21
#define LCDDB2           AT91C_PIO_PA22  
#define LCDDB3           AT91C_PIO_PA23    
#define LCDDB4           AT91C_PIO_PA15
#define LCDDB5           AT91C_PIO_PA16
#define LCDDB6           AT91C_PIO_PA24
#define LCDDB7           AT91C_PIO_PA25
#define LCDNWR           AT91C_PIO_PA18
#define LCDNRD           AT91C_PIO_PA19
#define LCDRS            AT91C_PIO_PA17
#define LCDNCS           AT91C_PIO_PA31
#define LCDNBLEN         AT91C_PIO_PA30
                         

/**
 * \def Power Management Controller 
 *
 *                                  
*/
#define PMC      AT91PS_PMC
#define PMC_BASE AT91C_BASE_PMC

/**
 * \def USB PIO
 * 
 *  
*/

//#define USB_PIO  AT91PS_PIO
//the -UOK signal from MAX8677:
//#define USB_PIO_BASE AT91C_BASE_PIOB


/**
 * \def OLED PIO
 * 
 *  
*/

//#define OLED_PIO_NENVOL	        AT91PS_PIO 
//#define OLED_PIO_NENVOL_BASE    AT91C_BASE_PIOA
//#define OLED_PIN_NENVOL         (1<<27)


#define BTN_PIO	    AT91PS_PIO 
#define BTN_BASE    AT91C_BASE_PIOA
#define BTN0 (1<<29)
#define BTN1 (1<<28)
#define BTN2 (1<<27)
#define BTN3 (1<<7)
#define BTN4 (1<<9)
/**
 * \name I/O pins
 * 
*/

//#define PIN_CARD_DETECT AT91C_PIO_PA18      //!< SD-card detect, 1=card inserted

//#define PIN_POWER_ON    AT91C_PIO_PA19      //!< Power_On Hold,0=enable
#define PIN_LED         DBG_PIN_RXD      //!< Power_On Hold,0=enable

//#define PIN_USB_PULLUP  AT91C_PIO_PA21      //!< USB pull-up enable, 0=enable
//#define PIN_USB_DETECT  AT91C_PIO_PB22      //!< USB bus voltage detection

//#define PIN_LCD_RESET   AT91C_PIO_PA15      //!< LCD Reset pin, 0=reset
//#define PIN_BTN_PRESS   AT91C_PIO_PA24      //!< Press button  
//#define PIN_BTN_UP1     AT91C_PIO_PA25      //!< Up button, inner  
//#define PIN_BTN_UP2     AT91C_PIO_PA26      //!< Up button, outer  
//#define PIN_BTN_DN1     AT91C_PIO_PA27      //!< Down button, inner  
//#define PIN_BTN_DN2     AT91C_PIO_PA28      //!< Down button, outer  
//#define PIN_VS_RESET    AT91C_PIO_PA16      //!< VS1033 Reset
//#define PIN_VS_DREQ     AT91C_PIO_PA17      //!< VS1033 DREQ
//@}

/**
 * \name Fast pointers to the internal peripherals
 * 
*/
//@{
//extern AT91PS_PIO  pPIOA;       //!< PIO controller
//extern AT91PS_PIO  pPIOB;       //!< PIO controller
//extern AT91PS_SPI   pSPI;       //!< SPI controller
//extern AT91PS_PMC   pPMC;       //!< Power Management controller
//@}

#endif /*HARDWARE_CONF_H_*/
