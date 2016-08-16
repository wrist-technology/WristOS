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
        config_panic.h  -  wire to an panic-reporting function

CONTAINS
        ABCSP_PANIC  -  report an panic from the abcsp code

DESCRIPTION
	If the abcsp code detects something disastrously wrong, typically
	something that is supposed to be "impossible", it can call
	ABCSP_PANIC().  The single argument reports the apparent disastrous
	occurrence.  This file defines the panic function.

	It is presumed external code will not call any of the abcsp code
	again until it has cleaned up the mess.

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __CONFIG_PANIC_H__
#define __CONFIG_PANIC_H__
 
/****************************************************************************
NAME
        ABCSP_PANIC  -  report an panic from the abcsp code

SYNOPSIS
        void ABCSP_PANIC(abcsp *_this, unsigned e);

FUNCTION
	Reports the occurrence of the panic "e".   Values for "e" are given
	in abcsp_panics.h.

NOTE
	Unlike ABCSP_EVENT(), it is not acceptable to #define this to be
	nothing.
*/
#define ABCSP_PANIC(t, n)        abcsp_panic((t), n)

#endif  /* __CONFIG_PANIC_H__ */
