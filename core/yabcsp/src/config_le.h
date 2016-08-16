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
        config_le.h  -  configure BCSP Link Establishment

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __CONFIG_LE_H__
#define __CONFIG_LE_H__
 

/* If this is #defined then the host uses the "passive-start" option.  This
is very unlikely to be of value.  There are systems where it makes sense to
use passive-start on the bc01 (e.g., the laptop which crashed if the bc01
sent any messages into its UART while Windows was booting), but the reverse
is unlikely.  A BCSP link must not use passive start on both sides of the
connection. */

/* #define      ABCSP_USE_BCSP_LE_PASSIVE_START */


/* If this is #defined then the host uses the old-style of BCSP Link
Establishment, described in CSR internal document bc01-s-010f.  Otherwise the
host uses the newer protocol, described in bc01-s-010g.  Further notes on the
two versions of the protocol are given in le.c.

This should be left un#defined unless the CSR chip is using a very old build
(before beta-10.X - approx March 2001). */

/* #define      ABCSP_USE_OLD_BCSP_LE */


/* This #define is ignored unless ABCSP_USE_OLD_BCSP_LE is #defined.

The maximum number of times the state machine will ask the peer if it
supports the ability to detect that the peer has restarted.  This can
normally be kept quite low - it only comes into play when the BCSP link is
established and flowing (choke has been removed).  The value is only more
than 1 to overcome the indeterminacy of using an unreliable BCSP channel.
Setting this to zero disables this feature - BCSP-LE never enters state
"garrulous." */

#define ABCSP_USE_BCSP_LE_CONF_CNT_LIMIT (4)


#endif  /* __CONFIG_LE_H__ */
