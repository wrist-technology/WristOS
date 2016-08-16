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
        txrx.h  -  information used by the transmit and receive paths

CONTAINS
        abcsp_txrx_init  -  initialise the two paths' common information

DESCRIPTION
        The transmit and receive paths of the abcsp library are fairly
        independent, but some data needs to be shared.  This file declares
        the shared information.

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __TXRX_H__
#define __TXRX_H__

/* Database of info common to tx and rx paths. */
struct txrx_info
{
	bool      choke:1;              /* Is the choke applied? */
	bool      txsync_req:1;         /* bcsp-le sync requested. */
	bool      txsyncresp_req:1;     /* bcsp-le syncresp requested. */
	bool      txconf_req:1;         /* bcsp-le conf requested. */
	bool      txconfresp_req:1;     /* bcsp-le confresp requested. */
	unsigned  rxseq_txack:3;        /* rxseq == txack. */
	bool      txack_req:1;          /* Request tx rxseq_txack. */
	unsigned  rxack:3;              /* Received acknowledgement. */
	bool      bcsple_muzzled:1;     /* bcsple passive start. */
	bool      rxdemux_bcsple_msg:1; /* rxdemux switch. */
	bool      txcrc:1;              /* Update crc on tx byte. */
};


/****************************************************************************
NAME
        abcsp_txrx_init  -  initialise the two paths' common information

FUNCTION
        Sets the block of data shared between the transmit and receive
        paths of the abcsp stack to its initial default values.
*/

extern void abcsp_txrx_init(abcsp *_this);


#endif  /* __TXRX_H__ */
