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
        txmsg.h  -  message transmission

CONTAINS
        abcsp_txmsg_init  -  initialise the transmit message assembler
        abcsp_sendmsg  -  set up message for sending to the uart
        abcsp_pumptxmsgs  -  send message to the uart

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __TXMSG_H__
#define __TXMSG_H__

#include "abcsp.h"

/* Message builder's state. */
enum txstate_enum {
	txstate_idle,           /* Waiting for new message to send. */
	txstate_msgstart,       /* Sending slip frame start byte. */
	txstate_hdr1,           /* Sending first bcsp header byte. */
	txstate_hdr2,           /* Sending second bcsp header byte. */
	txstate_hdr3,           /* Sending third bcsp header byte. */
	txstate_hdr4,           /* Sending last bcsp header byte. */
	txstate_payload,        /* Sending message payload. */
#ifdef  ABCSP_TXCRC
	txstate_crc1,           /* Sending first crc byte. */
	txstate_crc2,           /* Sending second crc byte. */
#endif
	txstate_msgend          /* Sending slip frame end byte. */
};
typedef enum txstate_enum txstate;


/* A message submitted to abcsp_sendmsg(). */
typedef struct txmsg
{
	ABCSP_TXMSG     *m;     /* The message (reference) itself. */
	uint            chan; /* BCSP channel. */
} TXMSG;

/* An internally-generated message (bcsp-le or ack). */
typedef struct
{
	uint8           *buf;     /* Message byte buffer. */
	uint            outdex:4; /* Buffer extraction point. */
	uint            chan:4;   /* BCSP channel. */
} INTMSG;

/* The message currently being transmitted. */
typedef struct
{
	uint            len;    /* Message length. */
	bool            rel:1;  /* Reliable message? */
/* Discriminated union. */
#define CURRMSG_TYPE_TXMSG      (0)
#define CURRMSG_TYPE_INTMSG     (1)
	unsigned        type:1; /* Discrim: TXMSG or internally-generated. */
	unsigned        seq:3;  /* Sequence number. */
	union {
		TXMSG   *txmsg;
		INTMSG  inter;
	} m;
} CURRMSG;

struct txmsg_state
{
	/* The ''state''. */
	txstate state;
  
	/* The current message */
	CURRMSG curr;

	/* Array of reliable abcsp_sendmsg() messages. */
	TXMSG rel[8];

	/* This is the sequence number of the next packet passed to
	 * us.  This is also the index into the circular buffer where
	 * we will add a new message.  No message actual has this
	 * sequence number at the moment. */
	uint msgq_txseq;

	/* This is the sequence number of the oldest packet that we
	 * sent that has not ackd yet.  Imagine that msgq_unackd_seq
	 * is less than or equal to msgq_txseq.  We have packets in
	 * this range (though not including msgq_txseq) in the
	 * retransmit Q. */
	uint msgq_unackd_txseq;

	/* Reliable packet sequence number - used by fsm to emit
	 * packets.  This is the sequence number that will be used for
	 * the next packet that we send. */
	uint txseq;

	/* Although we dont queue unreliable messages, we still need
	 * two slots here.  In one we store the message that we are
	 * sending, and in the other we store a new message that might
	 * be sent to us.
	 *
	 * unrel_index points to the slot that we can store a new
	 * message into.
	 */
	TXMSG unrel[2];
	uint unrel_index;

	/* BCSP packet header checksum. */
	uint8 cs;

	/* BCSP packet crc. */
	uint16 crc;
};


/****************************************************************************
NAME
        abcsp_txmsg_init  -  initialise the transmit message assembler

FUNCTION
        Initialises the transmit message assembler, abandoning any work
        in progress.

        This must be called before all other functions declared in this
        file.

        This may be called at any time to reinitialise the assembler.
*/

extern void abcsp_txmsg_init(abcsp *_this);


/****************************************************************************
NAME
        abcsp_txmsg_deinit  -  deinitialise the transmit message assembler

FUNCTION
*/

extern void abcsp_txmsg_deinit(abcsp *_this);


/****************************************************************************
NAME
        abcsp_sendmsg  -  set up message for sending to the uart
*/

/* Defined in abcsp.h. */


/****************************************************************************
NAME
        abcsp_pumptxmsgs  -  send message to the uart
*/

/* Defined in abcsp.h. */


#endif /* __TXMSG_H__ */
