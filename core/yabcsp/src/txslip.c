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
        txslip.c  -  send slip packets

CONTAINS
        abcsp_txslip_init  -  initialise the slip transmit block
        abcsp_txslip_msgdelim  -  send slip fame message delimiter to uart
        abcsp_txslip_sendbuf -  slip encode buffer and send to uart
        abcsp_txslip_flush  -  send buffer to uart
        abcsp_txslip_destroy  -  discard any message currently being slipped

$Revision: 1.2 $ by $Author: ca01 $
*/

#include "abcsp.h"


/****************************************************************************
NAME
        abcsp_txslip_init  -  initialise the slip encoder
*/

void abcsp_txslip_init(abcsp *_this)
{
    _this->txslip.escaping = FALSE;
    _this->txslip.ubufsiz = 0;
}


/****************************************************************************
NAME
        get_uart_buffer  -  slip encode buffer and send to uart
*/

static inline
int get_uart_buffer(abcsp *_this)
{
	if (_this->txslip.ubufsiz == 0)
	{
		_this->txslip.ubufsiz = ABCSP_UART_GETTXBUF(_this);
		_this->txslip.ubufindex = 0;
	}
	
	return _this->txslip.ubufsiz - _this->txslip.ubufindex;
}


/****************************************************************************
NAME
        abcsp_txslip_msgdelim  -  send slip fame message delimiter to uart
*/

bool abcsp_txslip_msgdelim(abcsp *_this)
{
    if (get_uart_buffer(_this) <= 0)
		return FALSE;
    
    _this->txslip.ubuf[_this->txslip.ubufindex++] = (uint8) 0xc0;
    return TRUE;
}


/****************************************************************************
NAME
        xabcsp_txslip_sendbuf -  slip encode buffer and send to uart
*/

static
uint16 xabcsp_txslip_sendbuf(abcsp *_this, uint8 *buf, uint16 bufsiz)
{
    uint nsent = 0;
    uint8 c;
    int spaces, spaces_at_start;
    uint8 *ubuf;

    spaces = spaces_at_start = get_uart_buffer(_this);
    ubuf = _this->txslip.ubuf + _this->txslip.ubufindex;

    if (_this->txslip.escaping && spaces)
    {
		--spaces;
		*ubuf++ = *buf == 0xc0 ? 0xdc : 0xdd;
		++buf;
		++nsent;
		_this->txslip.escaping = FALSE;
    }

    while(nsent < bufsiz && spaces)
    {
		--spaces;
		switch (c = *buf++)
		{
		case 0xc0:
			*ubuf++ = 0xdb;
			if (spaces != 0)
			{
			--spaces;
			*ubuf++ = 0xdc;
			break;
			}
			_this->txslip.escaping = TRUE;
			_this->txslip.ubufindex += spaces_at_start - spaces;
			return nsent;

		case 0xdb:
			*ubuf++ = 0xdb;
			if (spaces != 0)
			{
			--spaces;
			*ubuf++ = 0xdd;
			break;
			}
			_this->txslip.escaping = TRUE;
			_this->txslip.ubufindex += spaces_at_start - spaces;
			return nsent;

		default:
			*ubuf++ = c;
			break;
		}
		++nsent;
    }

    _this->txslip.ubufindex += spaces_at_start - spaces;
    return nsent;
}


/****************************************************************************
NAME
        abcsp_txslip_sendbuf -  slip encode buffer and send to uart
*/

uint16 abcsp_txslip_sendbuf(abcsp *_this, uint8 *buf, uint16 bufsiz)
{
    uint16 nsent = xabcsp_txslip_sendbuf(_this, buf, bufsiz);

#ifdef  ABCSP_TXCRC
    if (nsent > 0 && _this->txrx.txcrc)
		abcsp_crc_update(&_this->txmsg.crc, buf, nsent);
#endif  /* ABCSP_TXCRC */  
    
    return nsent;
}


/****************************************************************************
NAME
        abcsp_txslip_flush  -  send buffer to uart
*/

void abcsp_txslip_flush(abcsp *_this)
{
	if (_this->txslip.ubufsiz != 0)
	{
		ABCSP_UART_SENDBYTES(_this, _this->txslip.ubufindex);
		_this->txslip.ubufsiz = 0;
	}
}
