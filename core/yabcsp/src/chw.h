/* YABCSP - Yet Another BCSP

   Copyright (C) 2002 CSR
  
   Written 2002 by Mark Marshall <Mark.Marshall@csr.com>
  
   Use of this software is at your own risk. This software is
   provided "as is," and CSR cautions users to determine for
   themselves the suitability of using this software. CSR makes no
   warranty or representation whatsoever of merchantability or fitness
   of the product for any particular purpose or use. In no event shall
   CSR be liable for any consequential, incidental or special damages
   whatsoever arising out of the use of or inability to use this
   software, even if the user has advised CSR of the possibility of
   such damages.
*/
/****************************************************************************
FILE
        chw.h  -  hardware specific stuff

DESCRIPTION
    This should be #included by every C source file in the abcsp
	library.

    This file maps the code's common types to the local machine; it
    will need to be set for each target.

    This file should be changed only infrequently and with great care.

    This file should contain the minimum necessary to get the job done;
	it must not become a dumping ground for quick-fix globals.

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __CHW_H__
#define __CHW_H__
 
#if defined(__linux__) && defined(__i386__)

typedef signed char     int8;
typedef short           int16;
typedef long            int32;

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned long   uint32_t;
typedef long			int32_t;
typedef unsigned char   bool_t;

/*typedef unsigned int    uint;*/

#else /* linux/386 */
#if defined(__sun) && defined(__sparc) && defined(__SVR4)

typedef signed char     int8;
typedef short           int16;
typedef long            int32;

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned long   uint32_t;
typedef long			int32_t;
typedef unsigned char   bool_t;

typedef unsigned int    uint;

#else /* sun/sparc/SVR4 */
#if defined(__CYGWIN__)

typedef signed char     int8;
typedef short           int16;
typedef long            int32;

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned long   uint32_t;
typedef long			int32_t;
typedef unsigned char   bool_t;

typedef unsigned int    uint;

#else /* cygwin */
#if defined(_WIN32)

typedef signed char     int8;
typedef short           int16;
typedef long            int32;

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned long   uint32_t;
typedef long			int32_t;
typedef unsigned char   bool_t;

typedef unsigned int    uint;

#ifndef __cplusplus
#define inline
#endif

#else /* windows */
//MV #error "must define machine type"

// MV
#include "types.h"
/*
typedef signed char     int8;
typedef short           int16;
typedef long            int32;

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned long   uint32_t;
typedef long			int32_t;
typedef unsigned int    uint;
typedef unsigned char   bool_t;
*/


#endif /* windows */
#endif /* cygwin */
#endif /* sun/sparc/SVR4 */
#endif /* linux/386 */


#endif  /* __CHW_H__ */
