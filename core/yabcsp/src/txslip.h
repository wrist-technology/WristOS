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
        txslip.h  -  convert messages to slip and send them to the uart

CONTAINS
        abcsp_txslip_init  -  initialise the slip encoder
        abcsp_txslip_msgdelim  -  send slip fame message delimiter to uart
        abcsp_txslip_sendbuf -  slip encode buffer and send to uart
        abcsp_txslip_flush  -  send buffer to uart
        abcsp_txslip_destroy  -  discard any message currently being slipped

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __TXSLIP_H__
#define __TXSLIP_H__
 

#include "abcsp.h"


struct txslip_state
{
	bool escaping;
	unsigned ubufindex;
	unsigned ubufsiz;
	char ubuf[ABCSP_MAX_MSG_LEN + 4 + 2];
};

/****************************************************************************
NAME
        abcsp_txslip_init  -  initialise the slip encoder

FUNCTION
        Initialises the state of the txslip code block.

        This must be called before all other functions described in this
        file.

        This may be called at any time to reinitialise the state of the
        code block.
*/

extern void abcsp_txslip_init(abcsp *_this);


/****************************************************************************
NAME
        abcsp_txslip_msgdelim  -  send slip fame message delimiter to uart

RETURNS
	TRUE if the function has passed a BCSP frame delimiter byte (0xc0) to
	the UART buffer, else FALSE.
*/

extern bool abcsp_txslip_msgdelim(abcsp *_this);

/* The delimiter is used to mark the start and end of each frame. */
#define abcsp_txslip_msgstart(THIS)          abcsp_txslip_msgdelim(THIS)
#define abcsp_txslip_msgend(THIS)            abcsp_txslip_msgdelim(THIS)


/****************************************************************************
NAME
        abcsp_txslip_sendbuf -  slip encode buffer and send to uart

FUNCTION
        Slip-encodes and sends up to bufsiz bytes from "buf" to the UART.

RETURNS
        The number of bytes consumed from "buf".

NOTE
        Code under this function makes at most one call to
        ABCSP_UART_SENDBYTES().
*/

extern uint16 abcsp_txslip_sendbuf(abcsp *_this, uint8 *buf, uint16 bufsiz);


/****************************************************************************
NAME
        abcsp_txslip_flush  -  send buffer to uart

FUNCTION
        If the slip-encoder is holding a buffer obtained via a call to
        ABCSP_UART_GETBUF() then this is released by making a call to
        ABCSP_UART_SENDBYTES().
*/

extern void abcsp_txslip_flush(abcsp *_this);


/****************************************************************************
NAME
        abcsp_txslip_destroy  -  discard any message currently being slipped

FUNCTION
        Releases any resources currently held by the slip-encoder and
        reinitalises it.
*/

extern void abcsp_txslip_destroy(abcsp *_this);


#endif  /* __TXSLIP_H__ */
