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
        abcsp_panics.h  -  wire to an panic-reporting function

DESCRIPTION
	The macro ABCSP_PANIC() is used to route event information from the
	abcsp library to its environment.  This file defines the panic event
	codes used as arguments to ABCSP_PANIC().

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __ABCSP_PANICS_H__
#define __ABCSP_PANICS_H__
 

/* The abcsp library's transmit path checks that each message payload does
not exceed the limit #defined by ABCSP_MAX_MSG_LEN, set in config_txmsg.h.
This panic code is emitted if the limit is exceeded. */

#define ABCSP_PANIC_BCSP_MSG_OVERSIZE    ((unsigned)(1))


#endif  /* __ABCSP_PANICS_H__ */
