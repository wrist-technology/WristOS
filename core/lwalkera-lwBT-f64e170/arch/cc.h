/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __CC_H__
#define __CC_H__

#include "debug/trace.h"
#include "cpu.h"
#include "lwip/opt.h"

typedef uint8_t		u8_t;
#define U8_F		"d"
typedef int8_t		s8_t;
#define S8_F		"d"
typedef uint16_t	u16_t;
#define U16_F		"04x"
typedef int16_t		s16_t;
#define S16_F		"04x"
typedef uint32_t	u32_t;
#define U32_F		"08x"
typedef int32_t		s32_t;
#define S32_F		"08x"
typedef int			mem_ptr_t;
/*
typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   long    u32_t;
typedef signed     long    s32_t;
typedef u32_t mem_ptr_t;
*/

//MV
//typedef int sys_prot_t;

#if NO_SYS
//MV
struct sys_timeouts {
  struct sys_timeo *next;
};
#endif


#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

// MakingThings - add for lwip 1.3.0
// hmm...to be implemented?
//#define LWIP_PLATFORM_ASSERT(x) TRACE_ERROR(x) // fatal, print message and abandon execution.
//#define LWIP_PLATFORM_ASSERT(x) TRACE_BT(x);panic() // fatal, print message and abandon execution.
#define LWIP_PLATFORM_ASSERT(x) TRACE_BT(x) // fatal, print message and abandon execution.
//#define LWIP_PLATFORM_ASSERT(x) // fatal, print message and abandon execution.
#define LWIP_PLATFORM_DIAG(x)  TRACE_BT x // non-fatal, print a message
//#define LWIP_PLATFORM_DIAG(x) // non-fatal, print a message

// MV
#define U16LE2CPU(a) (*(a) | (*((a) + 1) << 8))
#define CPU2U16LE(a, v) *(a) = (u8_t)((v) & 0xff);*(a+1) = (u8_t)((v) >> 8);

#define U32LE2CPU(a) (*(a) | (*((a) + 1) << 8) | (*((a) + 2) << 16) | (*((a) + 3) << 24))
#define CPU2U32LE(a, v) *(a) = (u8_t)((v) & 0xff);*(a+1) = (u8_t)(((v) >> 8) & 0xff ); *(a+2) = (u8_t)(((v) >> 16) & 0xff );*(a+3) = (u8_t)(((v) >> 24) & 0xff );

#endif /* __CC_H__ */
