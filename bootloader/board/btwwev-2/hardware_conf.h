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

#include "AT91SAM7SE512.h"

#define BOARD_V2

#define SCR_COLORDEPTH 32

/**
 * \name Clock constants
 *
*/
//@{
#define EXT_OC          18432000   //!< External oscillator MAINCK
#define MCK             47923200   //!< MCK (PLLRC div by 2)
//@}

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
#define SPI_PIO_NPCS  AT91C_PA31_NPCS1

#define SDCARD_SPI_CHANNEL 1

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

#define USB_PIO  AT91PS_PIO
//the -UOK signal from MAX8677:
#define USB_PIO_BASE AT91C_BASE_PIOB


/**
 * \def OLED PIO
 * 
 *  
*/

#define OLED_PIO_NENVOL	        AT91PS_PIO 
#define OLED_PIO_NENVOL_BASE    AT91C_BASE_PIOA
#define OLED_PIN_NENVOL         (1<<27)
#define OLED_PIO_NORES          AT91PS_PIO
#define OLED_PIO_NORES_BASE     AT91C_BASE_PIOA
#define OLED_PIN_NORES          (1<<29)


#define NSRZZ_MASK (1<<28)

#define BUT0_MASK (1<<18)
#define BUT1_MASK (1<<31)
#define BUT2_MASK (1<<21)
#define BUT3_MASK (1<<24)
#define BUT4_MASK (1<<27)

#define CHARGEEN_PIN  (1<<28)
#define CHARGEEN_PIO_BASE AT91C_BASE_PIOA

#define PIN_CHARGING  AT91C_PIO_PA2

#define USBPEN2_PIN AT91C_PIO_PA21
#define PIN_CHARGEDONE AT91C_PIO_PA18
/**
 * \name I/O pins
 * 
*/

//#define PIN_CARD_DETECT AT91C_PIO_PA18      //!< SD-card detect, 1=card inserted

#define PIN_POWER_ON    AT91C_PIO_PA19      //!< Power_On Hold,0=enable
#define PIN_LED         AT91C_PIO_PA0      //!< Power_On Hold,0=enable

//#define PIN_USB_PULLUP  AT91C_PIO_PA21      //!< USB pull-up enable, 0=enable
#define PIN_USB_DETECT  AT91C_PIO_PB22      //!< USB bus voltage detection

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
