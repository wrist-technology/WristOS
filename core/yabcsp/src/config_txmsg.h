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
        config_txmsg.h  -  wire to the environment's tx message support

CONTAINS
        ABCSP_UART_GETTXBUF  -  obtain buffer for uart output
        ABCSP_UART_SENDBYTES  -  pass a block of bytes to the output uart
        ABCSP_TXMSG_INIT_READ  -  initialise reading a bcsp transmit message
        ABCSP_TXMSG_LENGTH  -  how long is a transmit message
        ABCSP_TXMSG_GETBUF  -  access raw message bytes in a message
        ABCSP_TXMSG_TAKEN  -  tell message how many bytes have been read
        ABCSP_TXMSG_DONE  -  signal that message has been delivered

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __CONFIG_TXMSG_H__
#define __CONFIG_TXMSG_H__
 
/* The size of the BCSP transmit window.  This must be between 1 and 7.  This
is normally set to 4.  This is called "winsiz" in the BCSP protocol
specification.

This determines the number of BCSP messages that can be handled by the abcsp
library's transmit path at a time, so it affects the storage requirements for
ABCSP_TXMSG messages. */

#define ABCSP_TXWINSIZE          (4)


/* If ABCSP_TXCRC is #defined then the optional CRC field is appended to each
BCSP message transmitted, else the CRC is not appended. */

#define ABCSP_TXCRC


/* The #define ABCSP_MAX_MSG_LEN sets the maximum number of bytes that can be
carried in a bcsp message payload.  This must be 4095 or smaller.  All real
systems use messages much smaller than this.  The abcsp library PANIC()s if
an attempt is made to send a message with a payload larger than this value.
See ABCSP_RXMSG_MAX_PAYLOAD_LEN in config_rxmsg.h. */

#define ABCSP_MAX_MSG_LEN        (4095)


/****************************************************************************
NAME
        ABCSP_UART_GETTXBUF  -  obtain buffer for uart output

SYNOPSIS
        size_t ABCSP_UART_GETTXBUF(abcsp *_this);

FUNCTION
        Obtains the size of a buffer into which to write UART output bytes.

		The YABCSP library works on an internal buffer allocated as part
		of the instance data. The buffer size is determined by the
		ABCSP_MAX_MSG_LEN value.

		This macro can be used to limit the size of the data passed onto
		the port driver.

		This macro is significantly different to the original ABCSP one.

RETURNS
        The maximum length of the output buffer.
*/
#define ABCSP_UART_GETTXBUF(t)    sizeof(t->txslip.ubuf)


/****************************************************************************
NAME
        ABCSP_UART_SENDBYTES  -  pass a block of bytes to the output uart

SYNOPSIS
        void ABCSP_UART_SENDBYTES(abcsp *_this, unsigned n);

FUNCTION
        Tells external code that it that must pass to the output UART the
        "n" bytes in the buffer (which is a part of the instance data).
*/
#define ABCSP_UART_SENDBYTES(t,n)        abcsp_uart_sendbytes((t),(n))


/****************************************************************************
NAME
        ABCSP_TXMSG_INIT_READ  -  initialise reading a bcsp transmit message

SYNOPSIS
        void ABCSP_TXMSG_INIT_READ(ABCSP_TXMSG *msg);

FUNCTION
		Tells the surrounding code that it wishes to start reading the
		message identified by "msg" from its start.

		The next call to ABCSP_TXMSG_GETBUF() is expected to obtain the first
		raw message bytes from "msg".
*/
#define ABCSP_TXMSG_INIT_READ(m)         abcsp_txmsg_init_read(m)


/****************************************************************************
NAME
        ABCSP_TXMSG_LENGTH  -  how long is a transmit message

SYNOPSIS
        unsigned ABCSP_TXMSG_LENGTH(ABCSP_TXMSG *msg);

RETURNS
        The number of bytes in the message "msg".
*/
#define ABCSP_TXMSG_LENGTH(m)            abcsp_txmsg_length(m)


/****************************************************************************
NAME
        ABCSP_TXMSG_GETBUF  -  access raw message bytes in a message

SYNOPSIS
        char *ABCSP_TXMSG_GETBUF(ABCSP_TXMSG *msg, unsigned *buflen);

RETURNS
	The address of a buffer containing the next raw message bytes to be
	read from "msg", or address zero (NULL) if all of the bytes have been
	read.

        If a buffer is returned its size is written at "buflen".
*/
#define ABCSP_TXMSG_GETBUF(m,l)          abcsp_txmsg_getbuf((m),(l))


/****************************************************************************
NAME
        ABCSP_TXMSG_TAKEN  -  tell message how many bytes have been read

SYNOPSIS
        void ABCSP_TXMSG_TAKEN(ABCSP_TXMSG *msg, unsigned ntaken);

FUNCTION
        Tells surrounding code that the abcsp library has read "ntaken"
        bytes from the buffer obtained from the preceding call to
        ABCSP_TXMSG_GETBUF().
*/
#define ABCSP_TXMSG_TAKEN(m,n)           abcsp_txmsg_taken((m),(n))


/****************************************************************************
NAME
        ABCSP_TXMSG_DONE  -  signal that message has been delivered

SYNOPSIS
        void ABCSP_TXMSG_DONE(abcsp *_this, ABCSP_TXMSG *msg);

FUNCTION
		Tells the surrounding code that the abcsp library has finished with
		"msg".  For unreliable messages this means it has been sent to the
		UART.  For reliable messages this means the peer BCSP stack has
		acknowledged reception of the message.
*/
#define ABCSP_TXMSG_DONE(t, m)           abcsp_txmsg_done((t),(m))

#endif  /* __CONFIG_TXMSG_H__ */
