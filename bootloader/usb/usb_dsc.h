/**
 * \file usb_dsc.h
 * Header: USB descriptor configuration 
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
#ifndef USB_DSC_H_
#define USB_DSC_H_

/** Converts an ASCII character to its Unicode equivalent */
#define USB_UNICODE(a)                      (a), 0x00

/**
 * Calculates the size of a string descriptor given the number of ASCII
 * characters in it
 * 
 */
#define USB_STRING_DESCRIPTOR_SIZE(size)    ((size * 2) + 2)

/** English (United States) */
#define USB_LANGUAGE_ENGLISH_US     0x0409

/**
 * This descriptor structure is used to provide information on
 * various parameters of the device
 * 
 */
typedef struct {

   unsigned char  bLength;              //!< Size of this descriptor in bytes
   unsigned char  bDescriptorType;      //!< DEVICE descriptor type
   unsigned short bscUSB;               //!< USB specification release number
   unsigned char  bDeviceClass;         //!< Class code
   unsigned char  bDeviceSubClass;      //!< Subclass code
   unsigned char  bDeviceProtocol;      //!< Protocol code
   unsigned char  bMaxPacketSize0;      //!< Control endpoint 0 max. packet size
   unsigned short idVendor;             //!< Vendor ID
   unsigned short idProduct;            //!< Product ID
   unsigned short bcdDevice;            //!< Device release number
   unsigned char  iManufacturer;        //!< Index of manu. string descriptor
   unsigned char  iProduct;             //!< Index of prod. string descriptor
   unsigned char  iSerialNumber;        //!< Index of S.N.  string descriptor
   unsigned char  bNumConfigurations;   //!< Number of possible configurations

#ifndef _DOXYGEN_
} __attribute__((packed)) S_usb_device_descriptor;
#else
} S_usb_device_descriptor;
#endif

/**
 * This is the standard configuration descriptor structure. It is used
 * to report the current configuration of the device.
 * 
 */ 
typedef struct {

   unsigned char  bLength;              //!< Size of this descriptor in bytes
   unsigned char  bDescriptorType;      //!< CONFIGURATION descriptor type
   unsigned short wTotalLength;         //!< Total length of data returned
                                        //!< for this configuration
   unsigned char  bNumInterfaces;       //!< Number of interfaces for this
                                        //!< configuration
   unsigned char  bConfigurationValue;  //!< Value to use as an argument for
                                        //!< the Set Configuration request to
                                        //!< select this configuration
   unsigned char  iConfiguration;       //!< Index of string descriptor
                                        //!< describing this configuration
   unsigned char  bmAttibutes;          //!< Configuration characteristics
   unsigned char  bMaxPower;            //!< Maximum power consumption of the
                                        //!< device
#ifndef _DOXYGEN_
} __attribute__((packed)) S_usb_configuration_descriptor;
#else
} S_usb_configuration_descriptor;
#endif                                        


/**
 * Standard interface descriptor. Used to describe a specific interface
 * of a configuration.
 * 
 */
typedef struct {

   unsigned char bLength;               //!< Size of this descriptor in bytes
   unsigned char bDescriptorType;       //!< INTERFACE descriptor type
   unsigned char bInterfaceNumber;      //!< Number of this interface
   unsigned char bAlternateSetting;     //!< Value used to select this alternate
                                        //!< setting
   unsigned char bNumEndpoints;         //!< Number of endpoints used by this
                                        //!< interface (excluding endpoint zero)
   unsigned char bInterfaceClass;       //!< Class code
   unsigned char bInterfaceSubClass;    //!< Sub-class
   unsigned char bInterfaceProtocol;    //!< Protocol code
   unsigned char iInterface;            //!< Index of string descriptor
                                        //!< describing this interface
#ifndef _DOXYGEN_                                        
} __attribute__((packed)) S_usb_interface_descriptor;
#else
} S_usb_interface_descriptor;
#endif

/**
 * This structure is the standard endpoint descriptor. It contains
 * the necessary information for the host to determine the bandwidth
 * required by the endpoint.
 * 
 */
typedef struct {

   unsigned char  bLength;              //!< Size of this descriptor in bytes
   unsigned char  bDescriptorType;      //!< ENDPOINT descriptor type
   unsigned char  bEndpointAddress;     //!< Address of the endpoint on the USB
                                        //!< device described by this descriptor
   unsigned char  bmAttributes;         //!< Endpoint attributes when configured
   unsigned short wMaxPacketSize;       //!< Maximum packet size this endpoint
                                        //!< is capable of sending or receiving
   unsigned char  bInterval;            //!< Interval for polling endpoint for
                                        //!< data transfers
#ifndef _DOXYGEN_                                        
} __attribute__((packed)) S_usb_endpoint_descriptor;
#else
} S_usb_endpoint_descriptor;
#endif

/**
 * The device qualifier structure provide information on a high-speed
 * capable device if the device was operating at the other speed.
 * 
 */ 
typedef struct {

   unsigned char  bLength;              //!< Size of this descriptor in bytes
   unsigned char  bDescriptorType;      //!< DEVICE_QUALIFIER descriptor type
   unsigned short bscUSB;               //!< USB specification release number
   unsigned char  bDeviceClass;         //!< Class code
   unsigned char  bDeviceSubClass;      //!< Sub-class code
   unsigned char  bDeviceProtocol;      //!< Protocol code
   unsigned char  bMaxPacketSize0;      //!< Control endpoint 0 max. packet size
   unsigned char  bNumConfigurations;   //!< Number of possible configurations
   unsigned char  bReserved;            //!< Reserved for future use, must be 0

#ifndef _DOXYGEN_
} __attribute__((packed)) S_usb_device_qualifier_descriptor;
#else
} S_usb_device_qualifier_descriptor;
#endif

/**
 * The S_usb_language_id structure represents the string descriptor
 * zero, used to specify the languages supported by the device. This
 * structure only define one language ID.
 * 
 */
typedef struct {

   unsigned char  bLength;               //!< Size of this descriptor in bytes
   unsigned char  bDescriptorType;       //!< STRING descriptor type
   unsigned short wLANGID;               //!< LANGID code zero

#ifndef _DOXYGEN_
} __attribute__((packed)) S_usb_language_id;
#else
} S_usb_language_id;
#endif

/**
 * Configuration descriptor used by the MSD driver
 * 
 * \see    S_usb_configuration_descriptor
 * \see    S_usb_interface_descriptor
 * \see    S_usb_endpoint_descriptor
 * 
 */
typedef struct {

    S_usb_configuration_descriptor sConfigurationDescriptor; //!< Configuration descriptor
    S_usb_interface_descriptor     sInterface;     //!< Interface descriptor
    S_usb_endpoint_descriptor      sBulkOut;       //!< Bulk OUT endpoint
    S_usb_endpoint_descriptor      sBulkIn;        //!< Bulk IN endpoint

} S_bot_configuration_descriptor;

/** List of standard descriptors used by the device */
typedef struct  {

    //! Device descriptor
    const S_usb_device_descriptor           *pDevice;
    //! Configuration descriptor
    const S_usb_configuration_descriptor    *pConfiguration;
#ifdef USB_HIGHSPEED
    //! Device qualifier descriptor
    const S_usb_device_qualifier_descriptor *pQualifier;
#endif    
    //! List of string descriptors
    const char                              **pStrings;
    //! List of endpoint descriptors
    const S_usb_endpoint_descriptor         **pEndpoints;
} S_std_descriptors;

extern const S_std_descriptors sDescriptors;

#endif  /*USB_DSC_H_*/
