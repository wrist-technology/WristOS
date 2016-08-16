/**
 * \file firmware_conf.h
 * Header: Firmware configuration
 * 
 * Configuration for all firmware related settings.
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
#ifndef FIRMWARE_CONF_H_
#define FIRMWARE_CONF_H_

#include <inttypes.h>   // include C99 standard types

#define SCR_COLORDEPTH 32

/**
 * Disable all debugging traces
 * 
 * Configure trace output in trace.h
*/
//#define NO_TRACES

/**
 * \def PRINTF_LEVEL
 * 
 * Compilation level of printf function.
 * 
 * PRINTF_MIN   Level 1 maintains a minimal version, just integer formatting
 * PRINTF_STD   Level 2 like above, with modifiers
 * PRINTF_FLT   Level 3 including floating point support
 * 
 */
#define PRINTF_LEVEL        PRINTF_STD

//#define NULL 0

#endif /*FIRMWARE_CONF_H_*/
