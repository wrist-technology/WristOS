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
        abcsp.h  -  another bcsp implementation

CONTAINS
        abcsp_init  -  initialise abcsp library
        abcsp_sendmsg  -  set up message for sending to the uart
        abcsp_pumptxmsgs  -  send message to the uart
        abcsp_uart_deliverbytes  -  push received uart bytes into the library
        abcsp_bcsp_timed_event  -  transmit path's timed event
        abcsp_tshy_timed_event  -  report a tshy timeout event to the fsm
        abcsp_tconf_timed_event  -  report a tconf timeout event to the fsm

DESCRIPTION
        This file describes the abcsp library's main external interfaces.

		See AN111, ABCSP Overview, for a detailed explanation.

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __ABCSP_H__
#define __ABCSP_H__

#include "commonc.h"
#include "chw.h"

/* This structure is only defined later */
typedef struct hci_bcsp abcsp;

#define ABCSP_RXMSG             void
#define ABCSP_TXMSG             void

#include "abcsp_support_functions.h"
#include <string.h>

#include "config_event.h"
#include "config_panic.h"
#include "config_timer.h"
#include "config_txmsg.h"
#include "config_rxmsg.h"
#include "config_le.h"

#include "txrx.h"
#include "rx.h"
#include "txmsg.h"
#include "txslip.h"
#include "le.h"
#include "crc.h"
#include "abcsp_panics.h"
#include "abcsp_events.h"

struct hci_bcsp
{	struct txmsg_state txmsg;

	/* Slip encoders state */
	struct txslip_state txslip;

	/* Slip decoder's state. */
	struct rxslip_state rxslip;

	/* Database of info common to tx and rx paths. */
	struct txrx_info txrx;

	struct le_state le;
};

/****************************************************************************
NAME
        abcsp_init  -  initialise abcsp library

FUNCTION
	Initialises the state of the abcsp library.  This resets all of its
	internal state, including FREE()ing held memory, cancelling any
	pending timed event timers and aborting the transfer of any messages
	in progress.

	This must be called before all other functions in the abcsp library,
	including all of the abcsp functions described in this file.

        This may be called at any time to reinitialise the state of the
        library.
*/

extern void abcsp_init(abcsp *_this);


/****************************************************************************
NAME
        abcsp_deinit  -  de-initialise abcsp library

FUNCTION
*/

extern void abcsp_deinit(abcsp *_this);


/****************************************************************************
NAME
        abcsp_sendmsg  -  set up message for sending to the uart

FUNCTION
	Passes the message "msg" into the library ready for transmission.
	The actual message translation and transmission is performed by
	abcsp_pumptxmsgs(), so this call really just parks "msg" within the
	library.

	The message is sent on BCSP channel "chan".  If "rel" is zero then it
	is sent as an unreliable datagram, else it is sent as a reliable
	datagram.

	Ownership of the memory referenced via "msg" is retained by the
	caller.  The caller must not alter the contents of "msg" until the
	abcsp library signals that it has completed its transmission (by
	calling ABCSP_TXMSG_DONE()) or until the library has been
	reinitialised.

RETURNS
        1 if the message was accepted for transmission, else 0.

	The function returns 0 if the initialisation function has not been
	called, if message transmission is currently being prevented (choke),
	if any argument is unacceptable or if the queue that would accept the
	message is full.
*/

extern unsigned abcsp_sendmsg(abcsp *_this, ABCSP_TXMSG *msg, unsigned chan, unsigned rel);


/****************************************************************************
NAME
        abcsp_pumptxmsgs  -  send message to the uart

FUNCTION
	Processes messages posted into the abcsp library by abcsp_sendmsg(),
	translating them to their wire SLIP form and sending this to the
	UART.  One or more calls to this function will be required to send
	each ABCSP_TXMSG.

	This function must not be called unless it is known that
	ABCSP_UART_SENDBYTES() can accept a block of bytes.

	One call to abcsp_pumptxmsgs() makes, at most, one call to
	ABCSP_UART_SENDBYTES().

	The function's return value can be used to decide on when next to
	call the function - it is an aid to the library's scheduling.  If it
	returns 0 then all immediate work on the library's transmit path is
	finished.  It returns 1 to indicate that there is work to be done as
	soon as possible.

RETURNS
	1 to indicate that the transmit path of the abcsp library still has
	work to do immediately, else 0.
*/

extern unsigned abcsp_pumptxmsgs(abcsp *_this);


/****************************************************************************
NAME
	abcsp_uart_deliverbytes  -  push received uart bytes into the library

FUNCTION
	Pushes slipped bytes received from the UART into the abcsp library.

	The "n" bytes in the buffer "buf" are passed into the library.  This
	may provoke a single call to ABCSP_DELIVERMSG().

	The caller retains ownership of "buf".

RETURNS
	The number of bytes taken from "buf".

	If this does not equal "n" it is normally reasonable to call the
	function again to try to deliver the remaining bytes.  However, if
	the function returns zero it implies the library failed to obtain a
	required resource (ABCSP_RXMSG or heap memory).
*/

extern unsigned abcsp_uart_deliverbytes(abcsp *_this, char *buf, unsigned n); 


/****************************************************************************
NAME
        abcsp_bcsp_timed_event  -  transmit path's timed event

FUNCTION
	The abcsp library's transmit path uses a single timed event
	controlled with ABCSP_START_BCSP_TIMER() and
	ABCSP_CANCEL_BCSP_TIMER().  If the timer fires it must call this
	function.

	The timer event provokes a retransmission of all unacknowledged
	transmitted BCSP messages.
*/

extern void abcsp_bcsp_timed_event(abcsp *_this);


/****************************************************************************
NAME
        abcsp_tshy_timed_event  -  report a tshy timeout event to the fsm

FUNCTION
        The bcsp link establishment engine uses two timers.  The first is
        controlled by ABCSP_START_TSHY_TIMER() and ABCSP_CANCEL_TSHY_TIMER().
        If this timer fires it must call this function.

        This function sends sends a Tshy timeout event into the bcsp-le
        state machine.
*/

extern void abcsp_tshy_timed_event(abcsp *_this);


/****************************************************************************
NAME
        abcsp_tconf_timed_event  -  report a tconf timeout event to the fsm

FUNCTION
	The bcsp link establishment engine uses two timers.  The second is
	controlled by ABCSP_START_TCONF_TIMER() and
	ABCSP_CANCEL_TCONF_TIMER().  If this timer fires it must call this
	function.

        This function sends sends a Tconf timeout event into the bcsp-le
        state machine.
*/

extern void abcsp_tconf_timed_event(abcsp *_this);


#endif  /* __ABCSP_H__ */
