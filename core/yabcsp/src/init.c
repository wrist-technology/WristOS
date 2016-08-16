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
        init.c  -  initialise the abcsp library

CONTAINS
        abcsp_init  -  initialise the abcsp library
        abcsp_deinit  -  de-initialise the abcsp library

$Revision: 1.2 $ by $Author: ca01 $
*/

#include "abcsp.h"


/****************************************************************************
NAME
        abcsp_init  -  initialise the abcsp library
*/

void abcsp_init(abcsp *_this)
{
	ABCSP_EVENT(_this, ABCSP_EVT_START);

	/* BCSP Link Establishment engine. */
	abcsp_bcsple_init(_this);

	/* The slip reception code block. */
	abcsp_uart_init(_this);

	/* Transmit message generator. */
	abcsp_txmsg_init(_this);

	/* Transmit message slip encoder. */
	abcsp_txslip_init(_this);

	/* We done */
	ABCSP_EVENT(_this, ABCSP_EVT_INITED);
}


/****************************************************************************
NAME
        abcsp_deinit  -  de-initialise the abcsp library
*/

void abcsp_deinit(abcsp *_this)
{
	/* BCSP Link Establishment engine. */
	abcsp_bcsple_deinit(_this);

	/* Transmit message generator. */
	abcsp_txmsg_deinit(_this);
}
