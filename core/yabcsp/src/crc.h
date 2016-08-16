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
        crc.h  -  cyclic redundancy check code

CONTAINS
        abcsp_crc_init  -  initialise the crc calculator
        abcsp_crc_update  -  update crc with next data byte
        abcsp_crc_reverse  -  translate crc into big-endian number

$Revision: 1.2 $ by $Author: ca01 $
*/

#ifndef __CRC_H__
#define __CRC_H__
 
#include "abcsp.h"


/****************************************************************************
NAME
        abcsp_crc_init  -  initialise the crc calculator

FUNCTION
        Write an initial value (0xffff) into *crc.
*/

extern void abcsp_crc_init(uint16 *crc);


/****************************************************************************
NAME
        abcsp_crc_update  -  update crc with next data byte

FUNCTION
        Updates the cyclic redundancy check value held in *crc with the
        next "len" data byte in the current sequence pointed to by "block"
*/

extern void abcsp_crc_update(uint16 *crc, const uint8 *block, int len);


/****************************************************************************
NAME
        abcsp_crc_reverse  -  translate crc into big-endian number

RETURNS
        A bit reversed version of crc.
*/

extern uint16 abcsp_crc_reverse(uint16 crc);


/****************************************************************************
NAME
        abcsp_crc_block  -  calculate a CRC over a block of data

RETURNS
	A crc the correct way round (no need to call abcsp_crc_reverse()).
*/

extern uint16 abcsp_crc_block(const uint8 *block, int len);


#endif /* __CRC_H__ */
