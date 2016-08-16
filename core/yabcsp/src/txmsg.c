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
        txmsg.c  -  transmit a bcsp packet

CONTAINS
        abcsp_txmsg_init  -  initialise the transmit message assembler
        abcsp_sendmsg  -  set up message for sending to the uart
        abcsp_pumptxmsgs  -  send message to the uart
        abcsp_bcsp_timed_event  -  transmit path's timed event

        pkt_avail  -  is there a new transmit message available
        pkt_cull  -  discard transmit messages no longer needed
        get_txmsgbuf  -  get a block of transmit message bytes
        txmsgbuf_taken  -  how many source bytes have been taken
        restart_timeout  -  stop and restart the bcsp timer
        slip_one_byte  -  slip one byte and send it to the uart

$Revision: 1.4 $ by $Author: ca01 $
*/

#include "abcsp.h"

/* Forward references. */
static unsigned xabcsp_pumptxmsgs(abcsp *_this);
static bool pkt_avail(abcsp *_this);
static void pkt_cull(abcsp *_this);
static uint8 *get_txmsgbuf(abcsp *_this, uint16 *len);
static void restart_timeout(abcsp *_this);
static void txmsgbuf_taken(abcsp *_this, uint16 ntaken);
static bool slip_one_byte(abcsp *_this, uint8 b);


/****************************************************************************
NAME
        abcsp_txmsg_init  -  initialise the transmit message assembler
*/

void abcsp_txmsg_init(abcsp *_this)
{
	uint seq;

    ABCSP_CANCEL_BCSP_TIMER(_this);

    _this->txmsg.state = txstate_idle;
    _this->txmsg.msgq_txseq = 0;
    _this->txmsg.msgq_unackd_txseq = 0;
    _this->txmsg.txseq = 0;
	_this->txmsg.unrel_index = 0;
	_this->txrx.rxseq_txack = 0;
	for (seq=0; seq<2; seq++)
		_this->txmsg.unrel[seq].m = NULL;
}


/****************************************************************************
NAME
        abcsp_txmsg_deinit  -  deinitialise the transmit message assembler
*/

void abcsp_txmsg_deinit(abcsp *_this)
{
	uint seq;

    ABCSP_CANCEL_BCSP_TIMER(_this);

        /* Clear the two message queues. */
	for (seq = _this->txmsg.msgq_unackd_txseq;
	     seq != _this->txmsg.msgq_txseq;
	     seq = (seq + 1) % 8)
	{
		ABCSP_TXMSG_DONE(_this, _this->txmsg.rel[seq].m);
	}

	for (seq=0; seq<2; seq++)
	{
		if (_this->txmsg.unrel[seq].m)
			ABCSP_TXMSG_DONE(_this, _this->txmsg.unrel[seq].m);
		_this->txmsg.unrel[seq].m = NULL;
	}
}


/****************************************************************************
NAME
        abcsp_sendmsg  -  set up message for sending to the uart
*/

unsigned abcsp_sendmsg(abcsp *_this, ABCSP_TXMSG *msg,
		       unsigned chan, unsigned rel)
{
        /* Reject all traffic if the choke is applied.

        BCSP-LE messages are transmitted from code below this entry point.

        The choke should be applied at the "mux" layer.  Applying it here
        means that if the choke is turned on while messages are queued for
        transmission then those messages will drain out.  This is strictly
        incorrect, but this won't harm any real system as the choke is only
        set TRUE by abcsp library init, so any peer is going to see
        disrupted traffic for a while anyway.  (Ideally, bcsp-le messages
        from here will tell the peer that we've restarted, so it should
        reinit and rechoke.) */

	if(_this->txrx.choke)
	{
                ABCSP_EVENT(_this, ABCSP_EVT_TX_CHOKE_DISCARD);
                return 0;
	}

        /* Parameter sanity checks. */
    if (rel > 1 || chan < 2 || chan > 15 || msg == NULL)
		return 0;

    if (rel)
	{
		/* We queue enough reliable messages to fill the
		 * WINSIZE window. */
		if (((_this->txmsg.msgq_txseq
		      - _this->txmsg.msgq_unackd_txseq) % 8)
		    >= ABCSP_TXWINSIZE)
		{
			ABCSP_EVENT(_this, ABCSP_EVT_TX_WINDOW_FULL_DISCARD);
			return 0;
		}

		/* We've checked the reliable queue has room. */
		_this->txmsg.rel[_this->txmsg.msgq_txseq].m = msg;
		_this->txmsg.rel[_this->txmsg.msgq_txseq].chan = chan;
		_this->txmsg.msgq_txseq = (_this->txmsg.msgq_txseq + 1) % 8;
	}
	else
	{
		ABCSP_TXMSG *old;

                /* The unreliable channel is biased towards supporting
		   sco, for which the data has to be fresh.  The queue
		   holds only one message, so we displace any message that's
		   already in the queue. */
		if ((old = _this->txmsg.unrel[_this->txmsg.unrel_index].m)
		    != NULL)
		{
	                ABCSP_TXMSG_DONE(_this,	old);
		}
		_this->txmsg.unrel[_this->txmsg.unrel_index].m = msg;
		_this->txmsg.unrel[_this->txmsg.unrel_index].chan = chan;
	}

    /* Tell external code that it needs to call abcsp_pumptxmsgs(). */
    ABCSP_REQ_PUMPTXMSGS(_this);

    /* Report message accepted. */
    return 1;
}


/****************************************************************************
NAME
        restart_timeout  -  stop and restart the bcsp timer

FUNCTION
        Cancels the current BCSP timed event timer, if it is running, then
        starts the BCSP timed timer.
*/

static inline
void restart_timeout(abcsp *_this)
{
    ABCSP_CANCEL_BCSP_TIMER(_this);
    ABCSP_START_BCSP_TIMER(_this);
}


/****************************************************************************
NAME
        slip_one_byte  -  slip one byte and send it to the uart

RETURNS
        TRUE if the byte "b" was slipped and sent to the UART, else FALSE.
*/

static inline
bool slip_one_byte(abcsp *_this, uint8 b)
{
	return abcsp_txslip_sendbuf(_this, &b, 1) == 1;
}


/****************************************************************************
NAME
        relq_contains  -  Does the retransmit Q conatin SEQ n

FUNCTION
        Returns TRUE if the retransmit Q contains a packet with the
        given sequence number.
*/

static inline
int relq_contains(abcsp *_this, uint8 num)
{
	if (_this->txmsg.msgq_txseq >= _this->txmsg.msgq_unackd_txseq)
	{
		return (num >= _this->txmsg.msgq_unackd_txseq
			&& num < _this->txmsg.msgq_txseq);
	}
	else
	{
		return (num >= _this->txmsg.msgq_unackd_txseq
			|| num < _this->txmsg.msgq_txseq);
	}
}
			

/****************************************************************************
NAME
        pkt_avail  -  is there a new transmit message available

FUNCTION
        This determines which of the available transmit messages should
        be sent next to the UART, i.e., it implements the message
        prioritisation.

        The function also configures this file's state ready for the
        chosen message's transmission.

RETURNS
        TRUE if a message is available to be sent, else FALSE.

        If the function returns TRUE then details of the packet to be sent
        have been written directly to this file's static variable "curr".
*/

static inline
bool pkt_avail(abcsp *_this)
{
    /* Default settings - an ack packet. */
    _this->txmsg.curr.rel = FALSE;
    _this->txmsg.curr.len = 0;
    _this->txmsg.curr.type = CURRMSG_TYPE_INTMSG;
    _this->txmsg.curr.m.inter.outdex = 0;
    _this->txmsg.curr.m.inter.chan = 0;
    _this->txmsg.curr.m.inter.buf = NULL;

    /* BCSP-LE messages have the highest priority.  (Always unrel.) */
    if(_this->txrx.txsync_req || _this->txrx.txsyncresp_req
	   || _this->txrx.txconf_req || _this->txrx.txconfresp_req)
	{
        _this->txmsg.curr.len = 4;
        _this->txmsg.curr.m.inter.chan = 1;
        if(_this->txrx.txsync_req)
		{
                        _this->txrx.txsync_req = FALSE;
                        _this->txmsg.curr.m.inter.buf =
				(uint8*)(abcsp_le_sync_msg);
		}
        else if(_this->txrx.txsyncresp_req)
		{
                        _this->txrx.txsyncresp_req = FALSE;
                        _this->txmsg.curr.m.inter.buf = 
				(uint8*)(abcsp_le_syncresp_msg);
		}
        else if(_this->txrx.txconf_req)
		{
                        _this->txrx.txconf_req = FALSE;
                        _this->txmsg.curr.m.inter.buf = 
				(uint8*)(abcsp_le_conf_msg);
		}
        else if(_this->txrx.txconfresp_req)
		{
			_this->txrx.txconfresp_req = FALSE;
			_this->txmsg.curr.m.inter.buf = 
				(uint8*)(abcsp_le_confresp_msg);
		}
		return TRUE;
	}
	
        /* Any unreliable TXMSG has the */
	if (_this->txmsg.unrel[_this->txmsg.unrel_index].m)
	{
		/* We *remove* the message from the queue.  We will
	   destroy it after sending it once.  (This is
	   different from the way in which relq is used, where
	   messages remain in the queue until their reception
	   is acknowledged by the peer.) */
        _this->txmsg.curr.type = CURRMSG_TYPE_TXMSG;
        _this->txmsg.curr.m.txmsg =
			&_this->txmsg.unrel[_this->txmsg.unrel_index];
        ABCSP_TXMSG_INIT_READ(_this->txmsg.curr.m.txmsg->m);
        _this->txmsg.curr.len =
		ABCSP_TXMSG_LENGTH(_this->txmsg.curr.m.txmsg->m);

		_this->txmsg.unrel_index =
			(_this->txmsg.unrel_index + 1) % 2;

        return TRUE;
	}
    /* Any reliable data is next in the pecking order. */
    if (relq_contains(_this, (uint8) _this->txmsg.txseq))
	{
        _this->txmsg.curr.type = CURRMSG_TYPE_TXMSG;
		_this->txmsg.curr.seq = _this->txmsg.txseq;
        _this->txmsg.curr.m.txmsg =
			&_this->txmsg.rel[_this->txmsg.txseq];

        ABCSP_TXMSG_INIT_READ(_this->txmsg.curr.m.txmsg->m);
        _this->txmsg.curr.len = 
			ABCSP_TXMSG_LENGTH(_this->txmsg.curr.m.txmsg->m);
        _this->txmsg.curr.rel = TRUE;

        _this->txmsg.txseq = (_this->txmsg.txseq + 1) % 8;
        restart_timeout(_this);

        return TRUE;
	}

    /* Finally, send an ack packet if needed. */
    if (_this->txrx.txack_req)
	{
		/* All messages send ack val, so clear the flag later.
		This also fights race hazard if tx and rx threads
		are separate. */
		return TRUE;
	}

	/* No message available. */
	return FALSE;
}


/****************************************************************************
NAME
        pkt_cull  -  discard transmit messages no longer needed

FUNCTION
        Determines which reliable packets have been received (acked)
        by the peer and discards the local copy of these if they are in
        the reliable transmit queue.
*/

static inline
void pkt_cull(abcsp *_this)
{
    uint rxack, seq;

    /* Give up immediately if there's nothing to cull. */
	if (_this->txmsg.msgq_txseq == _this->txmsg.msgq_unackd_txseq)
		return;

    /* The received ack value is always one more than the seq of
    the packet being acknowledged. */

	rxack = (_this->txrx.rxack - 1 + 8) & 7;

    /* abcsp_txrx.rxack carries the ack value from the last packet
     * received from the peer.  It marks the high water mark of
     * packets accepted by the peer.  We only use it if it refers
     * to a message in the transmit window, i.e., it should refer
     * to a message in relq.  (It's not always an error if it
     * refers to a message outside the window - it is initialised
     * that way - but it would be possible to extend this code to
     * detect absurd rxack values.)
	 *
	 * In the buffer relq packets in positions unackd_txseq to
	 * txseq-1 have been sent but not ackd.  Ie. there is
	 * something in this retransmit slot.  We walk through the
	 * unsent packets (from txseq to unackd_txseq-1), and if the
	 * recieved ack is in this set then ack should be ignored.
	 */

	if (!relq_contains(_this, (uint8) rxack))
		return;

    /* Finally, we walk relq, discarding its contents, until after
	 * we discard the packet with sequence number rxack.
	 *
	 * Seq counts through all of the packets that we have sent but
	 * not yet got an ack for.  If at some point seq == rxack the
	 * ack is in the range of packets that we have sent, and we
	 * need to clear some packets out of our retransmit buffer.
	 */
	while ((seq = _this->txmsg.msgq_unackd_txseq)
	       != _this->txmsg.msgq_txseq)
	{
		ABCSP_TXMSG_DONE(
			_this,
			_this->txmsg.rel[seq].m);

		_this->txmsg.msgq_unackd_txseq = (seq + 1) & 7;

		if (seq == rxack)
			break;
	}
}


/****************************************************************************
NAME
        get_txmsgbuf  -  get a block of transmit message bytes

FUNCTION
        Obtains a block of bytes from the currently selected transmit
        message.

RETURNS
        The base of the byte buffer to be transmitted, or NULL if no
        (more) bytes remain to be sent.

        The length of any returned buffer is written at "*len".
*/

static inline
uint8 *get_txmsgbuf(abcsp *_this, uint16 *len)
{
    unsigned ulen;
    uint8 *ret;

    /* Take from a plain byte buffer if an internally-generated msg. */
    if (_this->txmsg.curr.type == CURRMSG_TYPE_INTMSG)
	{
		if(_this->txmsg.curr.m.inter.outdex < _this->txmsg.curr.len)
		{
			*len = _this->txmsg.curr.len
				- _this->txmsg.curr.m.inter.outdex;
                        return &(_this->txmsg.curr.m.inter.buf
				 [_this->txmsg.curr.m.inter.outdex]);
		}
        return NULL;
	}

    /* Otherwise ask the environment for a buffer holding part
   of the whole message. */

    /* This should be one line, but lint moans. */
    ret = (uint8*)ABCSP_TXMSG_GETBUF(_this->txmsg.curr.m.txmsg->m,
				 &ulen);
	*len = (uint16)ulen;

    return ret;
}


/****************************************************************************
NAME
        txmsgbuf_taken  -  how many source bytes have been taken

FUNCTION
        Tells the current message source how many bytes have just been
        consumed.
*/

static inline
void txmsgbuf_taken(abcsp *_this, uint16 ntaken)
{
        if (_this->txmsg.curr.type == CURRMSG_TYPE_INTMSG)
                _this->txmsg.curr.m.inter.outdex += ntaken;
        else
                ABCSP_TXMSG_TAKEN(_this->txmsg.curr.m.txmsg->m, ntaken);
}


/****************************************************************************
NAME
        xabcsp_pumptxmsgs  -  send message to the uart

FUNCTION
        As abcsp_pumptxmsgs(), except that this does not send messages out
        to the UART via ABCSP_UART_SENDBYTES().  This function obtains a
        buffer via ABCSP_UART_GETBUF() and may put slipped bytes into this,
        but it does not actually push the buffer through to the UART.
*/

static inline
unsigned xabcsp_pumptxmsgs(abcsp *_this)
{
    uint8 *buf, b;
    uint16 buflen;

    for (;;)
	{
        switch(_this->txmsg.state)
		{
		default:
		case txstate_idle:
            /* Discard any acknowledged reliable messages. */
            pkt_cull(_this);

            /* Choose pkt to send.  Writes directly to "curr". */
            if (!pkt_avail(_this))
                    return 0;

            /* Sanity check. */
            if (_this->txmsg.curr.len > ABCSP_MAX_MSG_LEN)
                    ABCSP_PANIC(_this, 
			ABCSP_PANIC_BCSP_MSG_OVERSIZE);
            /* FALLTHROUGH */

		case txstate_msgstart:
            if (abcsp_txslip_msgstart(_this) == FALSE)
			{
				_this->txmsg.state = txstate_msgstart;
                                return 1;
			}
#ifdef  ABCSP_TXCRC
            abcsp_crc_init(&_this->txmsg.crc);
            _this->txrx.txcrc = TRUE;
#endif  /* ABCSP_TXCRC */
                        
			/* FALLTHROUGH */

		case txstate_hdr1:
			/* The txack value is 1 more than the seq of
			   the received packet being acknowledged.
			   However, the rxseq_txack value was
			   incremented by the receive path, so we use
			   the value of rxseq_txack directly. */

                b = _this->txrx.rxseq_txack << 3;
                _this->txrx.txack_req = FALSE;
                if (_this->txmsg.curr.rel)
                        b |= 0x80 + _this->txmsg.curr.seq;
#ifdef  ABCSP_TXCRC
                b |= 0x40;
#endif  /* ABCSP_TXCRC */
                _this->txmsg.cs = b;
                if(!slip_one_byte(_this, b))
				{
					_this->txmsg.state = txstate_hdr1;
									return 1;
				}
                        
			/* FALLTHROUGH */

		case txstate_hdr2:
            b = _this->txmsg.curr.len << 4;
            if (_this->txmsg.curr.type == CURRMSG_TYPE_INTMSG)
                    b |= _this->txmsg.curr.m.inter.chan;
            else
                    b |= _this->txmsg.curr.m.txmsg->chan;
            if (!slip_one_byte(_this, b))
			{
				_this->txmsg.state = txstate_hdr2;
                return 1;
			}
            _this->txmsg.cs += b;

            /* FALLTHROUGH */

		case txstate_hdr3:
            b = _this->txmsg.curr.len >> 4;
            if (!slip_one_byte(_this, b))
			{
				_this->txmsg.state = txstate_hdr3;
                return 1;
			}
            _this->txmsg.cs += b;
                        
			/* FALLTHROUGH */

		case txstate_hdr4:
            if (!slip_one_byte(_this, (uint8) ~_this->txmsg.cs))
			{
				_this->txmsg.state = txstate_hdr4;
                return 1;
			}

            /* FALLTHROUGH */

		case txstate_payload:

            while ((buf = get_txmsgbuf(_this, &buflen)) != NULL)
			{
				uint16 taken = abcsp_txslip_sendbuf(_this,
								 buf, buflen);
				txmsgbuf_taken(_this, taken);
				if (taken != buflen)
				{
					_this->txmsg.state = txstate_payload;
					return 1;
				}
			}

			/* Destroy the message if it's unreliable and
			   externally generated (main example: sco).
			   We send this sort of message only once, and
			   it's already been removed from the unrelq
			   in pkt_avail(). */

			if(_this->txmsg.curr.type == CURRMSG_TYPE_TXMSG
			   && _this->txmsg.curr.rel == FALSE)
			{
				ABCSP_TXMSG_DONE(
					_this,
					_this->txmsg.curr.m.txmsg->m);
				_this->txmsg.curr.m.txmsg->m = NULL;
			}
#ifdef ABCSP_TXCRC
			_this->txrx.txcrc = FALSE;
			_this->txmsg.crc
				= abcsp_crc_reverse(_this->txmsg.crc);

			/* FALLTHROUGH */

	    case txstate_crc1:
			if (!slip_one_byte(
				    _this,
				    (uint8)((_this->txmsg.crc >> 8) & 0xff)))
			{
				_this->txmsg.state = txstate_crc1;
				return 1;
			}
			
			/* FALLTHROUGH */
			
	    case txstate_crc2:
			if (!slip_one_byte(_this, 
					   (uint8)(_this->txmsg.crc & 0xff)))
			{
				_this->txmsg.state = txstate_crc2;
				return 1;
			}
#endif  /* ABCSP_TXCRC */

			/* FALLTHROUGH */

	    case txstate_msgend:
			if (abcsp_txslip_msgend(_this) == FALSE)
			{
				_this->txmsg.state = txstate_msgend;
				return 1;
			}
			_this->txmsg.state = txstate_idle;
			break;
	        }
        }

        /* LINTED Can't get here - this is just to shut lint up. */
        return 0;
}


/****************************************************************************
NAME
        abcsp_pumptxmsgs  -  send message to the uart
*/

unsigned abcsp_pumptxmsgs(abcsp *_this)
{
    unsigned ret;

    /* Prepare bytes for passing to the UART. */
    ret = xabcsp_pumptxmsgs(_this);

    /* Push (once!) the prepared bytes to the UART. */
    abcsp_txslip_flush(_this);

    /* Report whether any work remains to be done immediately. */
    return ret;
}


/****************************************************************************
NAME
        abcsp_bcsp_timed_event  -  transmit path's timed event
*/

void abcsp_bcsp_timed_event(abcsp *_this)
{
    uint8 new_txseq;

	if (_this->txmsg.msgq_txseq == _this->txmsg.msgq_unackd_txseq)
	{
		return;
	}

	/* Arrange to retransmit all messages in the relq. */
	if ((new_txseq = _this->txmsg.msgq_unackd_txseq) != _this->txmsg.txseq)
	{
		_this->txmsg.txseq = new_txseq;
		ABCSP_REQ_PUMPTXMSGS(_this);
		restart_timeout(_this);
	}
}
