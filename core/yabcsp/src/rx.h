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
        rx.h  -  the rx pipeline

CONTAINS
        abcsp_uart_init -  initialise the slip reception code block
        abcsp_uart_deliverbytes  -  push received uart bytes into the library

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __RXSLIP_H__
#define __RXSLIP_H__
 
#include "abcsp.h"


/* Slip decoder's state. */
enum rxslipstate_enum {
        rxslipstate_uninit,         /* Uninitialised. */
        rxslipstate_init_nosync,    /* First time unsynchronised. */
        rxslipstate_nosync,         /* No SLIP synchronisation. */
        rxslipstate_start,          /* Sending initial c0. */
        rxslipstate_body,           /* Sending message body. */
        rxslipstate_body_esc        /* Received escape character. */
};
typedef enum rxslipstate_enum rxslipstate;

struct rxslip_state
{
	rxslipstate state;
	unsigned index;
	char buf[ABCSP_RXMSG_MAX_PAYLOAD_LEN + 4 + 2];
};

/****************************************************************************
NAME
        abcsp_uart_init -  initialise the slip reception code block

FUNCTION
        Initialises the state of the rxslip code block.

        This must be called before all other functions described in this
        file.

        This may be called at any time to reinitialise the state of the
        code block.
*/

extern void abcsp_uart_init(abcsp *_this);


/****************************************************************************
NAME
        abcsp_uart_deliverbytes  -  push received uart bytes into the library
*/

/* Described and declared in abcsp.h. */


#endif /* __RXSLIP_H__ */
