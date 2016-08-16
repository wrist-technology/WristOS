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
        le.c  -  bcsp link establishment

CONTAINS
        abcsp_bcsple_init  -  initialise the link establishment engine
        abcsp_bcsple_putmsg  -  add a byte to a received bcsp-le message
        abcsp_tshy_timed_event  -  report a tshy timeout event to the fsm
        abcsp_tconf_timed_event  -  report a tconf timeout event to the fsm

        report_tshy_timeout  -  report a tshy timeout event to the fsm
        report_tconf_timeout  -  report a tconf timeout event to the fsm
        abcsp_le_fsm  -  the bcsp link establishment entity state machine
        match_lemsgid  -  match a link establishment message
        req_bcsple_msg  -  request transmission of a bcsp-le message

$Revision: 1.2 $ by $Author: ca01 $
*/

#include "abcsp.h"

const uint8 abcsp_le_sync_msg[]     = { 0xda, 0xdc, 0xed, 0xed };
const uint8 abcsp_le_syncresp_msg[] = { 0xac, 0xaf, 0xef, 0xee };
const uint8 abcsp_le_conf_msg[]     = { 0xad, 0xef, 0xac, 0xed };
const uint8 abcsp_le_confresp_msg[] = { 0xde, 0xad, 0xd0, 0xd0 };

/* Database mapping messages sent to/from the peer bcsp-le state machine
to message identifiers. */

typedef struct
{
    lemsgid         id;     /* Message identifier. */
    const uint8     *msg;   /* The message itself. */
} LEMSG;

static const LEMSG lemsgs[] =
{
    { lemsgid_sync,           abcsp_le_sync_msg },
    { lemsgid_sync_resp,      abcsp_le_syncresp_msg },
    { lemsgid_conf,           abcsp_le_conf_msg },
    { lemsgid_conf_resp,      abcsp_le_confresp_msg },
    { lemsgid_none,           ( const uint8* )( NULL ) }
};


/* Forward references. */
static void abcsp_lm_fsm(abcsp *_this, lemsgid msg);
static lemsgid match_lemsgid(const uint8 *buf, uint len);
static void req_bcsple_msg(abcsp *_this, lemsgid id);


/****************************************************************************
NAME
        abcsp_bcsple_init  -  initialise the link establishment entity
*/

void abcsp_bcsple_init(abcsp *_this)
{
    /* Attempt to prevent any existing timed events. */
    ABCSP_CANCEL_TCONF_TIMER(_this);
    ABCSP_CANCEL_TSHY_TIMER(_this);

	/* Start by seting most of the shared data to zero */
 	memset(&_this->txrx, 0x00, sizeof(struct txrx_info));
    _this->txrx.choke = TRUE;

   /* Configure the initial state of the bcsp-le state machine. */
#ifdef ABCSP_USE_BCSP_LE_PASSIVE_START
    _this->txrx.bcsple_muzzled = TRUE;
#else
    _this->txrx.bcsple_muzzled = FALSE;
#endif

    /* Stop most BCSP traffic flowing. */
    _this->txrx.choke = TRUE;

    /* Arrange to be called after a respectful interval. */
    ABCSP_START_TSHY_TIMER(_this);

    /* Emit the first sync message if not using passive-start. */
#ifndef ABCSP_USE_BCSP_LE_PASSIVE_START
	req_bcsple_msg(_this, lemsgid_sync);
#endif

    /* State machine's initial state. */
    _this->le.state = state_shy;
}


/****************************************************************************
NAME
        abcsp_bcsple_deinit  -  de-initialise the link establishment entity
*/

void abcsp_bcsple_deinit(abcsp *_this)
{
    /* Attempt to prevent any existing timed events. */
    ABCSP_CANCEL_TCONF_TIMER(_this);
    ABCSP_CANCEL_TSHY_TIMER(_this);
}


/****************************************************************************
NAME
        abcsp_bcsple_putmsg  -  process an entire bcsp-le message
*/

bool abcsp_bcsple_putmsg(abcsp *_this, const uint8 *buf, uint len)
{
    abcsp_lm_fsm(_this, match_lemsgid(buf, len));
	return TRUE;
}

/****************************************************************************
NAME
        abcsp_tshy_timed_event  -  report a tshy timeout event to the fsm
*/

void abcsp_tshy_timed_event(abcsp *_this)
{
    abcsp_lm_fsm(_this, lemsgid_tshy_timeout);
}


/****************************************************************************
NAME
        abcsp_tconf_timed_event  -  report a tconf timeout event to the fsm
*/

void abcsp_tconf_timed_event(abcsp *_this)
{
    abcsp_lm_fsm(_this, lemsgid_tconf_timeout);
}


/****************************************************************************
NAME
        abcsp_lm_fsm  -  the bcsp link establishment entity state machine

FUNCTION
        The message "msg" is fed into the bcsp-le state machine.
*/

static
void abcsp_lm_fsm(abcsp *_this, lemsgid msg)
{
    switch(_this->le.state)
	{
	case state_shy:
        switch(msg)
		{
		case lemsgid_tshy_timeout:
			if (!_this->txrx.bcsple_muzzled)
					req_bcsple_msg(_this, lemsgid_sync);
			ABCSP_START_TSHY_TIMER(_this);
			break;
		case lemsgid_sync:
			_this->txrx.bcsple_muzzled = FALSE;
            req_bcsple_msg(_this, lemsgid_sync_resp);
            break;
		case lemsgid_sync_resp:
            ABCSP_EVENT(_this, ABCSP_EVT_LE_SYNC);
#ifdef ABCSP_USE_OLD_BCSP_LE
            _this->txrx.choke = FALSE;
            _this->le.conf_cnt = 0;
#else
            req_bcsple_msg(_this, lemsgid_conf);
#endif
            ABCSP_START_TCONF_TIMER(_this);
            _this->le.state = state_curious;
            break;
		default:
			break;
		}
        break;
	case state_curious:
        switch(msg)
		{
		case lemsgid_tconf_timeout:
#ifdef ABCSP_USE_OLD_BCSP_LE
            if(_this->le.conf_cnt 
			   < ABCSP_USE_BCSP_LE_CONF_CNT_LIMIT) {
				++_this->le.conf_cnt;
				req_bcsple_msg(_this, lemsgid_conf);
				ABCSP_START_TCONF_TIMER(_this);
			}
#else
            req_bcsple_msg(_this, lemsgid_conf);
            ABCSP_START_TCONF_TIMER(_this);
#endif
            break;
		case lemsgid_sync:
            req_bcsple_msg(_this, lemsgid_sync_resp);
            break;
		case lemsgid_conf:
            req_bcsple_msg(_this, lemsgid_conf_resp);
            break;
		case lemsgid_conf_resp:
            ABCSP_EVENT(_this, ABCSP_EVT_LE_CONF);
#ifndef ABCSP_USE_OLD_BCSP_LE
            _this->txrx.choke = FALSE;
#endif
            _this->le.state = state_garrulous;
            break;
		default:
			break;
		}
        break;
	case state_garrulous:
        switch(msg)
		{
		case lemsgid_conf:
            req_bcsple_msg(_this, lemsgid_conf_resp);
            break;
		case lemsgid_sync:
            /* Peer has apparently restarted. */
            ABCSP_EVENT(_this, ABCSP_EVT_LE_SYNC_LOST);
			/* HERE Re-do the link establishment */
			abcsp_bcsple_init(_this);
			break;
		default:
			break;
		}
        break;
	default:
		break;
	}
}


/****************************************************************************
NAME
        match_lemsgid  -  match a link establishment message

RETURNS
        An identifier for the bcsp-le wire message at "msg", or
        lemsgid_none if the message could not be recognised.
*/

static
lemsgid match_lemsgid(const uint8 *buf, uint len)
{
    const LEMSG *m;

    /* The message should be 4 bytes long.  If so, match this against
    the expected message byte patterns and deliver any corresponding
    token to the bcsp-le state machine. */

    if (len == BCSPLE_MSGLEN)
        for (m = lemsgs ; m->msg != (const uint8*)(NULL) ; m++)
            if (memcmp(buf, m->msg, BCSPLE_MSGLEN) == 0)
                return m->id;

    return lemsgid_none;
}


/****************************************************************************
NAME
        req_bcsple_msg  -  request transmission of a bcsp-le message

FUNCTION
        Requests the transmit path of the abcsp library to emit one
        of the (fixed content) bcsp-le messages.
*/

static
void req_bcsple_msg(abcsp *_this, lemsgid id)
{
    /* <Sigh.>  The bitfield abcsp_txrx means we can't table-drive. */
    switch (id)
	{
	case lemsgid_sync:
		_this->txrx.txsync_req = 1;
		break;
	case lemsgid_sync_resp:
        _this->txrx.txsyncresp_req = 1;
        break;
	case lemsgid_conf:
        _this->txrx.txconf_req = 1;
        break;
	case lemsgid_conf_resp:
        _this->txrx.txconfresp_req = 1;
        break;
	default:
        return;
	}

	/* Kick the transmit path into wakefulness. */
	ABCSP_REQ_PUMPTXMSGS(_this);
}
