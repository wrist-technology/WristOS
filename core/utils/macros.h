/**
 * \file macros.h
 * Header: Macros
 * 
 * Usefull macro functions
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
#ifndef MACROS_H_
#define MACROS_H_

#include <types.h>

//! Boolean type
//MV typedef enum {false = 0, true = 1} bool;

//!  Generic callback function type
//
//         Since ARM Procedure Call standard allow for 4 parameters to be
//         stored in r0-r3 instead of being pushed on the stack, functions with
//         less than 4 parameters can be cast into a callback in a transparent
//         way.
typedef void (*Callback_f)(unsigned int, unsigned int,
                           unsigned int, unsigned int);

#define MIN(a, b) (((a) < (b)) ? (a) : (b)) //!< find and return minimum value of two integers

/** Returns the higher byte of a word */
#define HBYTE(word)                 ((unsigned char) ((word) >> 8))
/** Returns the lower byte of a word */
#define LBYTE(word)                 ((unsigned char) ((word) & 0x00FF))

/** Set flag(s) in a register */
#define SET(register, flags)        ((register) = (register) | (flags))
/** Clear flag(s) in a register */
#define CLEAR(register, flags)      ((register) &= ~(flags))

/** Poll the status of flags in a register */
#define ISSET(register, flags)      (((register) & (flags)) == (flags))
/** Poll the status of flags in a register */
#define ISCLEARED(register, flags)  (((register) & (flags)) == 0)

//! \brief  Converts a byte array to a word value using the big endian format
#define WORDB(bytes)            ((unsigned short) ((bytes[0] << 8) | bytes[1]))

//! \brief  Converts a byte array to a word value using the big endian format
#define WORDL(bytes)            ((unsigned short) ((bytes[1] << 8) | bytes[0]))

//! \brief  Converts a byte array to a dword value using the big endian format
#define DWORDB(bytes)   ((unsigned int) ((bytes[0] << 24) | (bytes[1] << 16) \
                                         | (bytes[2] << 8) | bytes[3]))

//! \brief  Converts a byte array to a dword value using the big endian format
#define DWORDL(bytes)   ((unsigned int) ((bytes[3] << 24) | (bytes[2] << 16) \
                                         | (bytes[1] << 8) | bytes[0]))

//! \brief  Stores a dword value in a byte array, in big endian format
#define STORE_DWORDB(dword, bytes) \
    bytes[0] = (unsigned char) ((dword >> 24) & 0xFF); \
    bytes[1] = (unsigned char) ((dword >> 16) & 0xFF); \
    bytes[2] = (unsigned char) ((dword >> 8) & 0xFF); \
    bytes[3] = (unsigned char) (dword & 0xFF);

//! \brief  Stores a word value in a byte array, in big endian format
#define STORE_WORDB(word, bytes) \
    bytes[0] = (unsigned char) ((word >> 8) & 0xFF); \
    bytes[1] = (unsigned char) (word & 0xFF);

#endif /*MACROS_H_*/
