/**
 * \file serial.c
 * Serial communication code
 * 
 * Code for serial communication via USART0 / DBGU
 * 
 * Several updates and port to AT91SAM7SE512 for Dynawa TCH1: Wrist Technology Ltd 2009  
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
#include <screen/screen.h>
#include "trace.h"
#include "serial.h"
#include <peripherals/pmc/pmc.h>

/**
 * \var pUSART
 * Pointers to the debug channel usart 
 * 
*/
 
DBG_USART pDBG_USART = DBG_USART_BASE;
DBG_PIO pDBG_PIO = DBG_PIO_BASE;  


/**
 * Init DBG_USART
 *  
 * Baudrate defined in hardware_conf.h
 * Parameters 8N1
 * 
 */
 
 
void dbg_usart_init(void)
{
       
    pDBG_PIO->PIO_PDR = DBG_PIN_TXD | DBG_PIN_RXD;   // Enable RxD TxD pin                                                       
                        
    pDBG_PIO->PIO_ASR =   DBG_PIN_TXD | DBG_PIN_RXD;      
                        
    pDBG_PIO->PIO_OER =   DBG_PIN_TXD;                                               

    #ifdef BOARD_V2        
    pDBG_USART->DBGU_CR =   AT91C_US_RSTRX |        // Reset receiver
                        AT91C_US_RSTTX |        // Reset transmitter
                        AT91C_US_RXDIS |        // Receiver disable
                        AT91C_US_TXDIS;         // Transmitter disable

    pDBG_USART->DBGU_MR =  DBG_USART_CONF;

    pDBG_USART->DBGU_BRGR = BAUDRATE_DIV;       // Baudrate divisor

    pDBG_USART->DBGU_CR =   AT91C_US_RXEN  |    // Receiver enable
                        AT91C_US_TXEN;      // Transmitter enable
    
    
    
    #elif defined(BOARD_V1)
    pPMC->PMC_PCER |= (1 << AT91C_ID_US1);
    
    pDBG_USART->US_CR =   AT91C_US_RSTRX |        // Reset receiver
                        AT91C_US_RSTTX |        // Reset transmitter
                        AT91C_US_RXDIS |        // Receiver disable
                        AT91C_US_TXDIS;         // Transmitter disable

    pDBG_USART->US_MR =  DBG_USART_CONF;

    pDBG_USART->US_BRGR = BAUDRATE_DIV;       // Baudrate divisor

    pDBG_USART->US_CR =   AT91C_US_RXEN  |    // Receiver enable
                        AT91C_US_TXEN;      // Transmitter enable
    #else
    #error "Specify board version"
    #endif                                               
}

/**
 * Send character via DBG_USART
 * 
 * \param   c   Character to send 
 * 
 */
void dbg_usart_putchar(char c)
{
    #ifdef BOARD_V2
    while (!(pDBG_USART->DBGU_CSR & AT91C_US_TXRDY));   // Wait for empty Tx buffer 
    pDBG_USART->DBGU_THR = c;
    
    #elif defined(BOARD_V1)    
    while (!(pDBG_USART->US_CSR & AT91C_US_TXRDY));   // Wait for empty Tx buffer 
    pDBG_USART->US_THR = c;
    
    #else
    #error "Specify board version"
    #endif
}

char dbg_usart_getchar(void)
{
    char c;
    #ifdef BOARD_V2
    while (!(pDBG_USART->DBGU_CSR & AT91C_US_RXRDY));   // Wait for empty Tx buffer 
    c=(char)pDBG_USART->DBGU_RHR;
    #ifdef DBGU_ECHO_ON
    dbg_usart_putchar(c);
    #endif
    return c;
    
    #elif defined(BOARD_V1)    
    while (!(pDBG_USART->US_CSR & AT91C_US_RXRDY));   // Wait for empty Tx buffer 
    c=(char)pDBG_USART->US_RHR;
    #ifdef DBGU_ECHO_ON
    dbg_usart_putchar(c);
    #endif
    return c;
    #else
    #error "Specify board version"
    #endif
}

