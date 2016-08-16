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
        config_timer.h  -  wire to the environment's timer functions

CONTAINS
        ABCSP_START_BCSP_TIMER  -  start the bcsp ack timeout timer
        ABCSP_START_TSHY_TIMER  -  start the bcsp-le tshy timer
        ABCSP_START_TCONF_TIMER  -  start the bcsp-le tconf timer
        ABCSP_CANCEL_BCSP_TIMER  -  cancel the bcsp ack timeout timer
        ABCSP_CANCEL_TSHY_TIMER  -  cancel the bcsp-le tshy timer
        ABCSP_CANCEL_TCONF_TIMER  -  cancel the bcsp-le tconf timer

DESCRIPTION
        The environment is required to provide a set of timers to support:

                bcsp's retransmission mechanism,
                bcsp link-establishment's Tshy timer
                bcsp link-establishment's Tconf timer

	External code must map to local timed event functions.  The three
	timers' periods are all bcsp configurable-items; these are also set
	in the external environment.  For example, a common value for the
	Tshy timer is 2 seconds, when the function supporting
	ABCSP_START_TSHY_TIMER() is called this should start a timer that
	calls abcsp_tshy_timed_event() after 2 seconds, unless
	ABCSP_CANCEL_TSHY_TIMER() is called first.

	The timers' accuracy requirements are lax; the only danger is the
	peer bcsp stack may think this stack is dead if events take too long
	to occur.  Accuracy to the nearest 0.1s is more than adequate.

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __CONFIG_TIMER_H__
#define __CONFIG_TIMER_H__


/****************************************************************************
NAMES
        ABCSP_START_BCSP_TIMER  -  start the bcsp ack timeout timer
        ABCSP_START_TSHY_TIMER  -  start the bcsp-le tshy timer
        ABCSP_START_TCONF_TIMER  -  start the bcsp-le tconf timer

SYNOPSES
        void ABCSP_START_BCSP_TIMER(abcsp *_this);
        void ABCSP_START_TSHY_TIMER(abcsp *_this);
        void ABCSP_START_TCONF_TIMER(abcsp *_this);

FUNCTIONS
        These three functions each schedule a timed event.  Each
        event catcher is "void fn(void)".

            The BCSP timer requires a call to abcsp_bcsp_timed_event();

            The TSHY timer requires a call to abcsp_tshy_timed_event();

            The TCONF timer requires a call to abcsp_tconf_timed_event();

        The timers' periods are configurable items, and are set in the
        external environment:

            The BCSP (Ttimeout) timer is normally set to 0.25s.

            The BCSP Link Establishment Tshy timer is normally set to 0.25s.

            The BCSP Link Establishment Tconf timer is normally set to 0.25s.

        The ABCSP_CANCEL_*_TIMER() functions each cancel the corresponding
        timer.

	The ABCSP_START_TSHY_TIMER() and ABCSP_START_TCONF_TIMER() timers are
	not used at the same time.
*/
#define ABCSP_START_BCSP_TIMER(t)  abcsp_start_bcsp_timer(t)
#define ABCSP_START_TSHY_TIMER(t)  abcsp_start_tshy_timer(t)
#define ABCSP_START_TCONF_TIMER(t) abcsp_start_tconf_timer(t)


/****************************************************************************
NAMES
        ABCSP_CANCEL_BCSP_TIMER  -  cancel the bcsp ack timeout timer
        ABCSP_CANCEL_TSHY_TIMER  -  cancel the bcsp-le tshy timer
        ABCSP_CANCEL_TCONF_TIMER  -  cancel the bcsp-le tconf timer

SYNOPSES
        void ABCSP_CANCEL_BCSP_TIMER(abcsp *_this);
        void ABCSP_CANCEL_TSHY_TIMER(abcsp *_this);
        void ABCSP_CANCEL_TCONF_TIMER(abcsp *_this);

FUNCTIONS
        Each function prevents its timed event from occurring, if possible.

        It is acceptable to call one of these cancel functions if the
        corresponding timer isn't running.
*/
#define ABCSP_CANCEL_BCSP_TIMER(t)  abcsp_cancel_bcsp_timer(t)
#define ABCSP_CANCEL_TSHY_TIMER(t)  abcsp_cancel_tshy_timer(t)
#define ABCSP_CANCEL_TCONF_TIMER(t) abcsp_cancel_tconf_timer(t)


#endif  /* __CONFIG_TIMER_H__ */
