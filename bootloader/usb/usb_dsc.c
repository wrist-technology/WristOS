/**
 * \file usb_dsc.c
 * USB descriptor configuration 
 * 
 * Descriptior changed: Wrist Technology Ltd
 *  - manuf
 *  - product ID
 *  - serial No. 
 *  - USB powered
 *  - 350mA consumption    
 *  
 * BASED ON:
 *   
 * USB descriptor configuration
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
#include "hardware_conf.h"
#include "firmware_conf.h"
#include <utils/macros.h>
#include "usb_drv.h"
#include "usb_msd.h"
#include "usb_sbc.h"
#include "usb_lun.h"
#include "usb_bot.h"
#include "usb_dsc.h"
#include "usb_std.h"

// Descriptors
//! Device descriptor
const S_usb_device_descriptor sDeviceDescriptor = {

    sizeof(S_usb_device_descriptor), // Size of this descriptor in bytes
    USB_DEVICE_DESCRIPTOR,           // DEVICE Descriptor Type
    0x0200,                          // USB Specification 2.0
    0x00,                            // Class is specified in the interface descriptor.
    0x00,                            // Subclass is specified in the interface descriptor.
    0x00,                            // Protocol is specified in the interface descriptor.
    EP0_BUFF_SIZE,                   // Maximum packet size for endpoint zero
    0x03EB,                          // Vendor ID "ATMEL"
    0x2002,/*0x6202,*/               // Product ID
    0x0100,                          // Device release number
    0x01,                            // Index 1: manufacturer string
    0x02,                            // Index 2: product string
    0x03,                            // Index 3: serial number string
    0x01                             // One possible configurations
};

//! Configuration descriptor
const S_bot_configuration_descriptor sConfigurationDescriptor = {

    // Configuration Descriptor
    {
        sizeof(S_usb_configuration_descriptor), // Size of this descriptor
        USB_CONFIGURATION_DESCRIPTOR,           // CONFIGURATION descriptor type
        sizeof(S_bot_configuration_descriptor), // Total size of descriptors
        0x01,                                   // One interface
        0x01,                                   // Configuration number 1
        0x00,                                   // No string description
        0x80,                                   // $change: Device is USB-powered
                                                // Remote wakeup not supported
        175,                                    // $change: 350mA consumption (charging)
    },
    // MSD Class Interface Descriptor
    {
        sizeof(S_usb_interface_descriptor), // Size of this descriptor in bytes
        USB_INTERFACE_DESCRIPTOR,           // INTERFACE descriptor type
        0x00,                               // Interface number 0
        0x00,                               // Setting 0
        2,                                  // Two endpoints used (excluding endpoint 0)
        MSD_INTF,                           // Mass storage class code
        MSD_INTF_SUBCLASS,                  // SCSI subclass code
        MSD_PROTOCOL,                       // Bulk-only transport protocol
        0x00                                // No string description
    },
    // Bulk-OUT Endpoint Descriptor
    {
        sizeof(S_usb_endpoint_descriptor),   // Size of this descriptor in bytes
        USB_ENDPOINT_DESCRIPTOR,             // ENDPOINT descriptor type
        0x01,                                // OUT endpoint, address 01h
        0x02,                                // Bulk endpoint
        BOT_OUT_EP_SIZE,                     // Maximum packet size is 64 bytes
        0x00,                                // Must be 0 for full-speed bulk
    },
    // Bulk_IN Endpoint Descriptor
    {
        sizeof(S_usb_endpoint_descriptor), // Size of this descriptor in bytes
        USB_ENDPOINT_DESCRIPTOR,           // ENDPOINT descriptor type
        0x82,                              // IN endpoint, address 02h
        0x02,                              // Bulk endpoint
        BOT_IN_EP_SIZE,                    // Maximum packet size if 64 bytes
        0x00,                              // Must be 0 for full-speed bulk
    }
};

//! Device qualifier descriptor
const S_usb_device_qualifier_descriptor sDeviceQualifierDescriptor = {

   sizeof(S_usb_device_qualifier_descriptor), // Size of this descriptor in bytes
   USB_DEVICE_QUALIFIER_DESCRIPTOR,     //!< DEVICE_QUALIFIER descriptor type
   0x0200,                              //!< USB specification release number
   MSD_INTF,                            //!< Class code
   MSD_INTF_SUBCLASS,                   //!< Sub-class code
   MSD_PROTOCOL,                        //!< Protocol code
   EP0_BUFF_SIZE,                       //!< Control endpoint 0 max. packet size
   1,                                   //!< Number of possible configurations
   0                                    //!< Reserved for future use, must be 0
};

// String descriptors
//! \brief  Language ID
const S_usb_language_id sLanguageID = {

    USB_STRING_DESCRIPTOR_SIZE(1),
    USB_STRING_DESCRIPTOR,
    USB_LANGUAGE_ENGLISH_US
};

//! \brief  Manufacturer description
const char pManufacturer[] = {

    USB_STRING_DESCRIPTOR_SIZE(10),
    USB_STRING_DESCRIPTOR,
    USB_UNICODE('D'),
    USB_UNICODE('y'),
    USB_UNICODE('n'),
    USB_UNICODE('a'),
    USB_UNICODE('w'),
    USB_UNICODE('a'),
    USB_UNICODE(' '),
    USB_UNICODE('L'),
    USB_UNICODE('T'),
    USB_UNICODE('D'),
};

//! \brief  Product descriptor
const char pProduct[] = {

    USB_STRING_DESCRIPTOR_SIZE(11),
    USB_STRING_DESCRIPTOR,
    USB_UNICODE('D'),
    USB_UNICODE('y'),
    USB_UNICODE('n'),
    USB_UNICODE('a'),
    USB_UNICODE('w'),
    USB_UNICODE('a'),
    USB_UNICODE('.'),
    USB_UNICODE('T'),
    USB_UNICODE('C'),
    USB_UNICODE('H'),
    USB_UNICODE('1'),    
};

//! \brief  Serial number
const char pSerial[] = {

    USB_STRING_DESCRIPTOR_SIZE(10),
    USB_STRING_DESCRIPTOR,
    USB_UNICODE('2'),
    USB_UNICODE('0'),
    USB_UNICODE('0'),
    USB_UNICODE('9'),
    USB_UNICODE('.'),
    USB_UNICODE('0'),
    USB_UNICODE('0'),
    USB_UNICODE('0'),
    USB_UNICODE('0'),
    USB_UNICODE('1'),    
};

//! \brief  List of string descriptors used by the device
const char *pStringDescriptors[] = {

    (char *) &sLanguageID,
    pManufacturer,
    pProduct,
    pSerial
};

//! \brief  List of descriptors used by the device
//! \see    S_std_descriptors
const S_std_descriptors sDescriptors = {

    &sDeviceDescriptor,
    (S_usb_configuration_descriptor *) &sConfigurationDescriptor,
#ifdef USB_HIGHSPEED    
    &sDeviceQualifierDescriptor,
#endif    
    pStringDescriptors,
    0
};
