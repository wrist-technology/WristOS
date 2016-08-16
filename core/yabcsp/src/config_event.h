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
        config_event.h  -  wire to an event-reporting function

CONTAINS
        ABCSP_EVENT  -  report an event from the abcsp code
        ABCSP_REQ_PUMPTXMSGS  -  request external code call abcsp_pumptxmsgs

DESCRIPTION
	Efficient operation of abcsp requires that abcsp code should be able
	to request external code to call abcsp_pumptxmsgs().  (The external
	code could just make repeated calls to the function, but this would
	be gross.)  When abcsp code detects that there is work to be done by
	a call to abcsp_pumptxmsgs() it calls ABCSP_REQ_PUMPTXMSGS().  This
	macro will normally set a flag in external code, provoking a
	subsequent call to the pump function.  Typical (crude) use would be
	something like:

                #define ABCSP_REQ_PUMPTXMSGS()  impl_req_pumptxmsgs()

                void impl_req_pumptxmsgs(void) { aflag = 1; }

        Then, in the external code's scheduler ...

                if(aflag) {
                        aflag = 0;
                        while(uart can accept bytes)
                                if(! abcsp_pumptxmsgs())
                                        break;
                        }

	The second macro, ABCSP_EVENT(), allows the abcsp code to signal
	occurrence of various events to the external code.  For example, the
	environment may wish to be told that an attempt to allocate heap/pool
	memory has failed, or that it has achieved BCSP-LE synchronisation.
	It is acceptable to #define this to be nothing.

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __CONFIG_EVENT_H__
#define __CONFIG_EVENT_H__
 

/****************************************************************************
NAME
        ABCSP_EVENT  -  report an event from the abcsp code

SYNOPSIS
        void ABCSP_EVENT(abcsp *_this, unsigned e);

FUNCTION
        Reports the occurrence of the event "e".   Values for "e" are
	given in abcsp_events.h.

NOTE
	It is acceptable to #define ABCSP_EVENT() to be nothing.  This should
	cause all calls to the macro to drop silently from the code.
*/
#define ABCSP_EVENT(t, n)           abcsp_event((t), (n))


/****************************************************************************
NAME
        ABCSP_REQ_PUMPTXMSGS  -  request external code call abcsp_pumptxmsgs

SYNOPSIS
        void ABCSP_REQ_PUMPTXMSGS(abcsp *_this);

FUNCTION
	Tells external code that there is work to be done on the abcsp
	library's transmit path, and requests that it make a call to
	abcsp_pumptxmsgs() at its earliest convenience.

NOTE
        This macro must not be wired directly to abcsp_pumptxmsgs() or the
        code will go re-entrant and bomb.  This function should normally set
        a flag that should provoke a call to abcsp_pumptxmsgs() after the
        current call into the abcsp library has returned.
*/
#define ABCSP_REQ_PUMPTXMSGS(t)  abcsp_req_pumptxmsgs((t))


#endif  /* __CONFIG_EVENT_H__ */
