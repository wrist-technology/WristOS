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
        commonc.h  -  system-wide common header file

DESCRIPTION
        This should be #included by every C source file in the library.

        The file includes typedefs and #defines that are common to many
        C projects.

        This file should be changed only infrequently and with great care.

		This file should contain the bare minimum necessary to get the job
		done; it should not become a dumping ground for quick-fix globals.

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __COMMONC_H__
#define __COMMONC_H__
 
#undef  NULL
#define NULL            (0)

#undef  bool
#define bool            unsigned

#undef  fast
#define fast            register

#undef  TRUE
#define TRUE            (1)

#undef  FALSE
#define FALSE           (0)

#undef  forever
#define forever         for(;;)

#undef  max
#define max(a,b)        (((a) > (b)) ? (a) : (b))

#undef  min
#define min(a,b)        (((a) < (b)) ? (a) : (b))

/* To shut lint up. */
#undef  unused
#define unused(x)       (void)x


#endif  /* __COMMONC_H__ */
