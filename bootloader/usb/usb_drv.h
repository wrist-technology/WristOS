/**
 * \file usb_drv.h
 * Header: USB driver 
 * 
 * USB driver functions
 * 
 * FIXIES, UPDATES, PORTS, ANSI-C: Wrist Technology Ltd 
 * 
 * BASED ON:  
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
#ifndef USB_DRV_H_
#define USB_DRV_H_

/* Highspeed device is NOT supported on AT91SAM7-128/256 */
#ifdef USB_HIGHSPEED
#undef USB_HIGHSPEED
#endif

/** Interrupt mask */
#define ISR_MASK            0x00003FFF

#define NUM_OF_ENDPOINTS     3  //!< Number of used endpoint, including EP0

/**
 * \name Values returned by the API methods
 *  
 */
//@{
//! Last method has completed successfully
#define USB_STATUS_SUCCESS      0

//! Method was aborted because the recipient (device, endpoint, ...) was busy
#define USB_STATUS_LOCKED       1

//! Method was aborted because of abnormal status
#define USB_STATUS_ABORTED      2

//! Method was aborted because the endpoint or the device has been reset
#define USB_STATUS_RESET        3

//! Method status unknow
#define USB_STATUS_UNKOWN       4
//@}

/**
 * \name Constant values used to track which USB state the device is currently in.
 * 
 */
//@{
//! Detached state
#define USB_STATE_DETACHED                          (1 << 0)
//! Attached state
#define USB_STATE_ATTACHED                          (1 << 1)
//! Powered state
#define USB_STATE_POWERED                           (1 << 2)
//! Default state
#define USB_STATE_DEFAULT                           (1 << 3)
//! Address state
#define USB_STATE_ADDRESS                           (1 << 4)
//! Configured state
#define USB_STATE_CONFIGURED                        (1 << 5)
//! Suspended state
#define USB_STATE_SUSPENDED                         (1 << 6)
//@}

/** 
 * \name UDP endpoint configuration
 * 
*/
//@{
#define EP0                 0   //!< Endpoint 0
#define EP0_BUFF_SIZE       8   //!< Endpoint 0 size
#define EP1                 1   //!< Endpoint 1
#define EP1_BUFF_SIZE       64  //!< Endpoint 1 size
#define EP2     2               //!< Endpoint 2
#define EP2_BUFF_SIZE       64  //!< Endpoint 2 size
#define EP3     3               //!< Endpoint 3
#define EP3_BUFF_SIZE       64  //!< Endpoint 3 size
//@}

/**
 * Returns the index of the last set (1) bit in an integer
 * 
 * \param  value Integer value to parse
 * \return Position of the leftmost set bit in the integer
 * 
 */
extern signed char last_set_bit(unsigned int value);
/*
{
    signed char locindex = -1;

    if (value & 0xFFFF0000)
    {
        locindex += 16;
        value >>= 16;
    }

    if (value & 0xFF00)
    {
        locindex += 8;
        value >>= 8;
    }

    if (value & 0xF0)
    {
        locindex += 4;
        value >>= 4;
    }

    if (value & 0xC)
    {
        locindex += 2;
        value >>= 2;
    }

    if (value & 0x2)
    {
        locindex += 1;
        value >>= 1;
    }

    if (value & 0x1)
    {
        locindex++;
    }

    return locindex;
}
*/
/** Clear flags of UDP UDP_CSR register and waits for synchronization */
#define usb_ep_clr_flag(pInterface, endpoint, flags) { \
while (pInterface->UDP_CSR[endpoint] & (flags)) \
pInterface->UDP_CSR[endpoint] &= ~(flags); \
}

/** Set flags of UDP UDP_CSR register and waits for synchronization */
#define usb_ep_set_flag(pInterface, endpoint, flags) { \
while ( (pInterface->UDP_CSR[endpoint] & (flags)) != (flags) ) \
pInterface->UDP_CSR[endpoint] |= (flags); \
}

/** \enum usb_state_e
 * Enumeration of USB device states
 * 
 */
typedef enum usb_state_e
{
    DETACHED_STATE,
    ATTACHED_STATE,
    POWERED_STATE,
    DEFAULT_STATE,
    ADDRESS_STATE,
    CONFIGURED_STATE,
    SUSPENDED_STATE   
} usb_state_e;


/**
 * Possible endpoint states
 * 
 */
typedef enum {

    endpointStateDisabled,
    endpointStateIdle,
    endpointStateWrite,
    endpointStateRead,
    endpointStateHalted

} endpoint_state_e;

/**
 * Structure for data include in USB requests
 * 
 */
typedef struct
{
    unsigned char   bmRequestType:8;    //!< Characteristics of the request
    unsigned char   bRequest:8;         //!< Particular request
    unsigned short  wValue:16;          //!< Request-specific parameter
    unsigned short  wIndex:16;          //!< Request-specific parameter
    unsigned short  wLength:16;         //!< Length of data for the data phase

} __attribute__((packed)) s_usb_request;

/**
 * Structure for endpoint transfer parameters
 * 
 */
typedef struct {

    // Transfer descriptor
    char                    *pData;             //!< \brief Transfer descriptor
                                                //!< pointer to a buffer where
                                                //!< the data is read/stored
    unsigned int            dBytesRemaining;    //!< \brief Number of remaining
                                                //!< bytes to transfer
    unsigned int            dBytesBuffered;     //!< \brief Number of bytes
                                                //!< which have been buffered
                                                //!< but not yet transferred
    unsigned int            dBytesTransferred;  //!< \brief Number of bytes
                                                //!< transferred for the current
                                                //!< operation
    Callback_f              fCallback;          //!< \brief Callback to invoke
                                                //!< after the current transfer
                                                //!< is complete
    void                    *pArgument;         //!< \brief Argument to pass to
                                                //!< the callback function                                                
    // Hardware information
    unsigned int            wMaxPacketSize;     //!< \brief Maximum packet size
                                                //!< for this endpoint
    unsigned int            dFlag;              //!< \brief Hardware flag to
                                                //!< clear upon data reception
    unsigned char           dNumFIFO;           //!< \brief Number of FIFO
                                                //!< buffers defined for this
                                                //!< endpoint
    unsigned int   dState;                      //!< Endpoint internal state
#ifndef _DOXYGEN_ 
} __attribute__((packed)) s_usb_endpoint;
#else
} s_usb_endpoint;
#endif

extern volatile unsigned char usb_device_state; //!< Device status, connected/disconnected etc.
extern volatile unsigned char usb_configuration;
 

unsigned char usb_check_bus_status(void);

//new, debounced:
unsigned char usb_power_detect(void);

void usb_soft_disable_device(void);
void usb_bus_reset_handler(void);
void usb_endpoint_handler(unsigned char endpoint);
void usb_stall(unsigned char endpoint);
bool usb_halt(unsigned char endpoint, unsigned char request);
char usb_write(unsigned char endpoint,
               const void *pData,
               unsigned int len,
               Callback_f    fCallback,
               void          *pArgument);
char usb_read(unsigned char endpoint,
              void *pData,
              unsigned int len,
              Callback_f    fCallback,
              void          *pArgument);
inline char usb_send_zlp0(Callback_f  fCallback, void *pArgument);
void usb_set_address(void);
void usb_configure_endpoint(unsigned char endpoint);
void usb_set_configuration(void);

#endif /*USB_DRV_H_*/
