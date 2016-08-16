/**
 * \file serial.h
 * Header: Serial communication
 * 
 * Serial communication via DBG USART (select us0, us1, dbgu)
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
#ifndef SERIAL_H_
#define SERIAL_H_

/**
 * \def BAUDRATE_DIV
 * Baudrate Divisor 
 * 
*/
#define BAUDRATE_DIV    (MCK/16/DBG_USART_BAUDRATE)

/**
 * \def DBG
 * Function name used for USRT0 output stream
 * 
 */
#define DBG     dbg_usart_putchar

#define DBGU_ECHO_ON

void dbg_usart_init(void);
void dbg_usart_putchar(char c);
char dbg_usart_getchar(void);

#endif /*SERIAL_H_*/
