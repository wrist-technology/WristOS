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
        le.h  -  bcsp link establishment

CONTAINS
        abcsp_bcsple_init  -  initialise the link establishment engine
        abcsp_bcsple_newmsg  -  create a bcsp-le message
        abcsp_bcsple_putbyte  -  add a byte to a received bcsp-le message
        abcsp_bcsple_flush  -  push outstanding bytes back to bcsp-le message
        abcsp_bcsple_done  -  signal that bcsp-le message is complete
        abcsp_bcsple_destroy  -  abandon construction of bcsp-le message

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __LE_H__
#define __LE_H__

#include "abcsp.h"


/* BCSP-LE state machine states. */
enum bcsp_le_state_enum {
        state_shy,
        state_curious,
        state_garrulous
};
typedef enum bcsp_le_state_enum bcsp_le_state;


/* LE message identifiers - signals sent to/from bcsp-le state machines. */
enum lemsgid_enum {
        lemsgid_none,           /* no message */
        lemsgid_sync,           /* sync message */
        lemsgid_sync_resp,      /* sync-resp message */
        lemsgid_conf,           /* conf message */
        lemsgid_conf_resp,      /* conf-resp message */
        lemsgid_tshy_timeout,   /* message indicating Tshy timeout */
        lemsgid_tconf_timeout   /* message indicating Tconf timeout */
};
typedef enum lemsgid_enum lemsgid;

/* Messages sent to/from the peer bcsp-le state machine.  All messages
are the same length, simplifying some of the code.   Messages are global
as they are used by the receive and transmit paths*/

#define BCSPLE_MSGLEN           (4)

/* BCSP-LE messages are all fixed-format. */
extern const uint8 abcsp_le_sync_msg[];
extern const uint8 abcsp_le_syncresp_msg[];
extern const uint8 abcsp_le_conf_msg[];
extern const uint8 abcsp_le_confresp_msg[];

struct le_state
{
	bcsp_le_state state;

#ifdef ABCSP_USE_OLD_BCSP_LE
	uint8 conf_cnt;
#endif
};


/****************************************************************************
NAME
        abcsp_bcsple_init  -  initialise the link establishment engine

FUNCTION
        Initialises the BCSP Link Establishment engine, abandoning any
        work in progress.

        This must be called before all other functions declared in this
        file.

        This may be called at any time to reinitialise the engine.
*/

extern void abcsp_bcsple_init(abcsp *_this);


/****************************************************************************
NAME
        abcsp_bcsple_deinit  -  deinitialise the link establishment engine

FUNCTION
*/

extern void abcsp_bcsple_deinit(abcsp *_this);


/****************************************************************************
NAME
        abcsp_bcsple_putbyte  -  Process an entire bcsp-le message

RETURNS
	TRUE if all went well, FALSE if something went wrong.
*/

extern bool abcsp_bcsple_putmsg(abcsp *_this, const uint8 *buf, uint len);


#endif /* __LE_H__ */
