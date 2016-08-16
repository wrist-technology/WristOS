/**
 * \file usb_lun.h
 * Header: LUN functions
 * 
 * Logical Unit Number (LUN) functions
 * 
 * AT91SAM7S-128 USB Mass Storage Device with SD Card by Michael Wolf\n
 * Copyright (C) 2008 Michael Wolf\n\n
 * 
 * This program is free software: you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation, either version 3 of the License, or\n
 * any later version.\n\n
 * 
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 * GNU General Public License for more details.\n\n
 * 
 * You should have received a copy of the GNU General Public License\n
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 * 
 */
#ifndef USB_LUN_H_
#define USB_LUN_H_

#define LUN_STATUS_SUCCESS          0x00    //!< LUN operation success
#define LUN_STATUS_ERROR            0x02    //!< LUN operation error

/** \struct S_lun 
 * LUN description structure
 * 
 */ 
typedef struct {

    S_sbc_inquiry_data          *pInquiryData;      //!< Inquiry data structur
    unsigned char               *pReadWriteBuffer;  //!< Pointer to LUN read/write buffer
    S_sbc_request_sense_data    sRequestSenseData;  //!< Sense data structure
    S_sbc_read_capacity_10_data sReadCapacityData;  //!< Capacity data sturcture
    //unsigned long               dSize;              //!< Size of LUN in bytes
    unsigned long long               dSize;              //!< Size of LUN in bytes
    unsigned int                dBlockSize;         //!< Sector size of LUN in bytes
    unsigned char               bMediaStatus;       //!< LUN status

} S_lun;

extern S_lun pLun[1]; 

void lun_init(unsigned char *pBuffer,
              //unsigned long  dSize,
              unsigned long long  dSize,
              unsigned int   dBlockSize);
unsigned char lun_read(unsigned long dBlockAddress,
                       void         *pData,
                       unsigned int dLength,
                       Callback_f   fCallback,
                       void         *pArgument);
unsigned char lun_write(unsigned long dBlockAddress,
                        void         *pData,
                        unsigned int dLength,
                        Callback_f   fCallback,
                        void         *pArgument);                       
#endif /*USB_LUN_H_*/
