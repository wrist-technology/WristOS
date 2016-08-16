/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#ifndef USBSERIAL_H
#define USBSERIAL_H

#include "config.h"
#ifdef MAKE_CTRL_USB
#include "rtos.h"

#include "board.h"
#include "CDCDSerialDriver.h"
#include "CDCDSerialDriverDescriptors.h"

#define MAX_INCOMING_SLIP_PACKET 400
#define MAX_OUTGOING_SLIP_PACKET 600
#define USB_SER_RX_BUF_LEN BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CDCDSerialDriverDescriptors_DATAIN)

// class Semaphore;

/**
  Virutal serial port USB communication.
  This allows the Make Controller look like a serial modem to your desktop, which it can then easily
  open up, read and write to.
  
  \section Usage
  There's only one UsbSerial object in the system, so you never create your own.  You can access
  it through the get() method.
  
  \code
  UsbSerial* usb = UsbSerial::get(); // first get a reference to the central UsbSerial object
  usb->write("hello", 5); // write a little something
  
  char buffer[128];
  int got = usb->read(buffer, 128); // and read
  \endcode
  
  \section Drivers
  On OS X, the system driver is used - no external drivers are needed.
  An entry in \b /dev is created - similar to <b>/dev/cu.usbmodem.xxxx</b>.  It may be opened for reading and 
  writing like a regular file using the standard POSIX open(), close(), read(), write() functions.

  On Windows, the first time the device is seen, it needs 
  to be pointed to a .INF file containing additional information - the \b make_controller_kit.inf in 
  the same directory as this file.  Once Windows sets this up, the device can be opened as a normal
  COM port.  If you've installed mchelper or mcbuilder, this file is already in the right spot
  on your system.
*/

typedef struct {
  Semaphore readSemaphore;
  Semaphore writeSemaphore;
  int justGot, rxBufCount;
  int justWrote;
  char rxBuf[USB_SER_RX_BUF_LEN];
  char slipOutBuf[MAX_OUTGOING_SLIP_PACKET];
  char slipInBuf[MAX_INCOMING_SLIP_PACKET];
} UsbSerial; 

void UsbSerial_open( void );
void UsbSerial_close(void);
bool UsbSerial_isActive( void );
//int UsbSerial_read( char *buffer, int length, int timeout = -1);
int UsbSerial_read( char *buffer, int length, int timeout );
int UsbSerial_write( const char *buffer, int length, int timeout );
int UsbSerial_readSlip( char *buffer, int length );
int UsbSerial_writeSlip( const char *buffer, int length );

#endif // MAKE_CTRL_USB

#endif // USBSERIAL_H
