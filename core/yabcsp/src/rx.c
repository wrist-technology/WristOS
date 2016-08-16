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
        rx.c  -  receive message pipeline

CONTAINS
        abcsp_uart_deliverbytes  -  push received uart bytes into the library

        abcsp_rxmsg_putmsg  -  process an entire received message
        abcsp_rxmsgdemux_putmsg  -  process an entire BCSP message
        abcsp_rxbcsp_putmsg  -  decode and entire bcsp message
        abcsp_uart_init -  initialise the slip reception code block

DESCRIPTION
        This receives data from the uart and converts it into packets.
        There s one function for each layer (deslipping, bcsp decode,
        demux and delivery).

$Revision: 1.2 $ by $Author: ca01 $
*/

#include "abcsp.h"
#include "debug/trace.h"


/****************************************************************************
NAME
        abcsp_rxmsg_putmsg  -  process an entire received message
*/

static inline
bool abcsp_rxmsg_putmsg(abcsp *_this, uint chan, bool rel, uint len,
			const uint8 *in_buf)
{
    ABCSP_RXMSG *msg;
    char *out_buf;
    unsigned buflen;
    
    if ((msg = ABCSP_RXMSG_CREATE(_this, len)) == NULL)
		return FALSE;

    while(len > 0)
    {
		if ((out_buf = ABCSP_RXMSG_GETBUF(msg, &buflen)) == NULL)
		{
			ABCSP_RXMSG_DESTROY(_this, msg);
			return FALSE;
		}

		if (buflen > len)
			buflen = len;
		memcpy(out_buf, in_buf, buflen);
		
		ABCSP_RXMSG_WRITE(msg, out_buf, buflen);
		
		len -= buflen;
		in_buf += buflen;
    }

    ABCSP_RXMSG_COMPLETE(_this, msg);
    ABCSP_DELIVERMSG(_this, msg, (unsigned) chan, (unsigned) rel);

    return TRUE;
}


/****************************************************************************
NAME
        abcsp_rxmsgdemux_putmsg  -  process an entire BCSP message
*/

static inline
bool abcsp_rxmsgdemux_putmsg(abcsp *_this, uint chan, bool rel, uint len,
			     const uint8 *buf)
{    
    /* Is this a BCSP Link Establishment message? */
    if (chan == 1 && rel == 0)
		return abcsp_bcsple_putmsg(_this, buf, len);
    else
		return abcsp_rxmsg_putmsg(_this, chan, rel, len, buf);
}


/****************************************************************************
NAME
        abcsp_rxbcsp_putmsg  -  decode and entire bcsp message
*/

static inline
void abcsp_rxbcsp_putmsg(abcsp *_this, const uint8 *msg, int len)
{
    uint hdr_len, hdr_chan, hdr_ack, hdr_seq;
    bool hdr_rel, hdr_crc_present;

    if (len < 4)
    {
		ABCSP_EVENT(_this, ABCSP_EVT_SHORT_PAYLOAD);
		return;
    }

    if (((msg[0] + msg[1] + msg[2] + msg[3]) & 0xff) != 0xff)
    {
		ABCSP_EVENT(_this, ABCSP_EVT_CHECKSUM);
		return;
    }

    hdr_rel = (msg[0] & 0x80) != 0;
    hdr_crc_present = (msg[0] & 0x40) != 0;
    hdr_ack = (msg[0] >> 3) & 0x07;
    hdr_seq = msg[0] & 0x07;
    hdr_chan = msg[1] & 0x0f;
    hdr_len = ((msg[1] >> 4) & 0x0f) | ((uint16)msg[2] << 4);

    if (_this->txrx.choke && !(hdr_chan == 1 && !hdr_rel))
    {
		ABCSP_EVENT(_this, ABCSP_EVT_RX_CHOKE_DISCARD);
		return;
    }

#ifdef ABCSP_RXMSG_MAX_PAYLOAD_LEN
    if (hdr_len > ABCSP_RXMSG_MAX_PAYLOAD_LEN)
    {
		ABCSP_EVENT(_this, ABCSP_EVT_OVERSIZE_DISCARD);
		return;
    }
#endif

    if (hdr_len + 4 + (hdr_crc_present ? 2 : 0) != (uint) len)
    {
		if (hdr_len + 4 + (hdr_crc_present ? 2 : 0) < (uint) len)
			ABCSP_EVENT(_this, ABCSP_EVT_OVERRUN);
		else
			ABCSP_EVENT(_this, ABCSP_EVT_SHORT_PAYLOAD);
		return;
    }

#ifdef ABCSP_RXCRC
    if (hdr_crc_present &&
		abcsp_crc_block(msg, len-2) != (msg[len-2] << 8) + msg[len-1])
	{
		ABCSP_EVENT(_this, ABCSP_EVT_CRC_FAIL);
		return;
    }
#endif /* ABCSP_RXCRC */

    if (hdr_rel) {
        TRACE_BT("msg_seq %d bcsp_seq %d\r\n", hdr_seq, _this->txrx.rxseq_txack);
    }
    if (hdr_rel && hdr_seq != _this->txrx.rxseq_txack)
    {
		ABCSP_EVENT(_this, ABCSP_EVT_MISSEQ_DISCARD);
		
		/* BCSP must acknowledge all reliable packets to avoid deadlock. */
		_this->txrx.txack_req = 1;
		
		/* Wake the tx path so that it can discard the acknowledged
		   message(s). */
		ABCSP_REQ_PUMPTXMSGS(_this);
    }
    else
    {
		/* We don't deliver ack msgs (chan zero). */
		if(hdr_chan != 0)
		{					
			if(!abcsp_rxmsgdemux_putmsg(_this, hdr_chan, hdr_rel, hdr_len,
						msg+4))
			return;
		}
		
		/* If the message is reliable we need to note the next rel rxseq we
		   will accept.  This is numerically identical to the value that we
		   send back to the peer in outbound packets' ack fields to tell the
		   host that we've got this message, i.e., the ack value sent to the
		   peer is one more than the packet being acknowledged, modulo 8.
		*/
		if (hdr_rel)
		{
			_this->txrx.rxseq_txack = (_this->txrx.rxseq_txack + 1) % 8;
			_this->txrx.txack_req = 1;

			/* Wake the tx path to send the new ack val back to the peer. */
			ABCSP_REQ_PUMPTXMSGS(_this);
		}
    }

    /* We accept rxack acknowledgement info from any intact packet,
       reliable or unreliable.  This includes reliable messages with the
       wrong seq number. */
    
    if (hdr_ack != _this->txrx.rxack)
    {
		_this->txrx.rxack = hdr_ack;

		/* Wake the tx path so that it can discard the
		   acknowledged message(s). */
		
		ABCSP_REQ_PUMPTXMSGS(_this);
    }
}


/****************************************************************************
NAME
        abcsp_uart_init -  initialise the slip reception code block
*/

void abcsp_uart_init(abcsp *_this)
{
    _this->rxslip.state = rxslipstate_init_nosync;
	_this->rxslip.index = 0;
}


/****************************************************************************
NAME
        abcsp_uart_deliverbytes  -  push received uart bytes into the library

FUNCTION
        Pushes the UART bytes into the slip decoder and then through
        to the rxbcsp (bcsp message analyser) engine.

RETURNS
	The number of bytes consumed.

	This function will stop if it cannot consume a byte or if ir
	reaches the end of a message.
*/

unsigned abcsp_uart_deliverbytes(abcsp *_this, char *ibuf, unsigned len)
{
    uint8 * const buf = (uint8 *) ibuf;
    uint8 c, *p = buf, *end = buf + len;
    
    if (len == 0)
	return 0;

    c = *p++;
	
    switch(_this->rxslip.state)
    {
		default:
		case rxslipstate_uninit:
			ABCSP_EVENT(_this, ABCSP_EVT_UNINITED);
			return 0;
		case rxslipstate_init_nosync:
			if(c != 0xc0)
				return 1;
			ABCSP_EVENT(_this, ABCSP_EVT_SLIP_SYNC);
			/* FALLTHROUGH */
		case rxslipstate_nosync:
			if(c != 0xc0)
			{
				_this->rxslip.state = rxslipstate_init_nosync;
				return 1;
			}
			if (p == end)
			{
				_this->rxslip.state = rxslipstate_start;
				return 1;
			}
			c = *p++;
			/* FALLTHROUGH */
		case rxslipstate_start:
			while (c == 0xc0)
				if (p == end)
				{
					_this->rxslip.state = rxslipstate_start;
					return p - buf;
				}
				else
					c = *p++;
			_this->rxslip.state = rxslipstate_body;
			_this->rxslip.index = 0;
			/* FALLTHROUGH. */
			do {
		case rxslipstate_body:
			if(c == 0xc0)
			{
				abcsp_rxbcsp_putmsg(_this, _this->rxslip.buf,
							_this->rxslip.index);
				/* Deliver message and signal "no more" to UART. */
				_this->rxslip.state = rxslipstate_nosync;
				return p - buf;
			}
			else if(c != 0xdb)
			{
				if (_this->rxslip.index >= (ABCSP_RXMSG_MAX_PAYLOAD_LEN+4+2))
				{
					ABCSP_EVENT(_this, ABCSP_EVT_OVERSIZE_DISCARD);
					_this->rxslip.state = rxslipstate_nosync;
					return p - buf;
				}
				_this->rxslip.buf[_this->rxslip.index++] = c;
				continue;
			}
			if (p == end)
			{
				_this->rxslip.state = rxslipstate_body_esc;
				return p - buf;
			}
			c = *p++;
			/* FALLTHROUGH */
		case rxslipstate_body_esc:
			if(c == 0xdc)
			{
				if (_this->rxslip.index >= (ABCSP_RXMSG_MAX_PAYLOAD_LEN+4+2))
				{
					ABCSP_EVENT(_this, ABCSP_EVT_OVERSIZE_DISCARD);
					_this->rxslip.state = rxslipstate_nosync;
					return p - buf;
				}
				_this->rxslip.buf[_this->rxslip.index++] = (uint8) 0xc0;
			}
			else if(c == 0xdd)
			{
				if (_this->rxslip.index >= (ABCSP_RXMSG_MAX_PAYLOAD_LEN+4+2))
				{
					ABCSP_EVENT(_this, ABCSP_EVT_OVERSIZE_DISCARD);
					_this->rxslip.state = rxslipstate_nosync;
					return p - buf;
				}
				_this->rxslip.buf[_this->rxslip.index++] = (uint8) 0xdb;
			}
			else
			{
				/* Byte sequence error.  Abandon current message. */
				ABCSP_EVENT(_this, ABCSP_EVT_SLIP_SYNC_LOST);
				_this->rxslip.state = rxslipstate_init_nosync;
				return p - buf;
			}
			_this->rxslip.state = rxslipstate_body;
		} while(p < end && (c = *p++, 1));
		break;
    }
        
    return p - buf;
}
