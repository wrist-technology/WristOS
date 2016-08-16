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

#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"
#include "rtos.h"
//#include "AT91SAM7X256.h"
#include "AT91SAM7SE512.h"

#define DMA     1

#define SERIAL_PORTS 2
#define SERIAL_DEFAULT_BAUD        9600
#define SERIAL_DEFAULT_BITS        8
#define SERIAL_DEFAULT_STOPBITS    1
#define SERIAL_DEFAULT_PARITY      0
#define SERIAL_DEFAULT_HANDSHAKING 0
#define SERIAL_RX_BUF_SIZE         100

/**
  Send and receive data via the Make Controller's serial ports.

  There are 2 full serial ports on the Make Controller, and this library provides support for both of them.

  Control all of the common serial characteristics including:
  - \b baud - the speed of the connection (110 - > 2M) in baud or raw bits per second.  9600 baud is the default setting.
  - \b bits - the size of each character (5 - 8).  8 bits is the default setting.
  - \b stopBits - the number of stop bits transmitted (1 or 2)  1 stop bit is the default setting.
  - \b parity - the parity policy (-1 is odd, 0 is none and 1 is even).  Even is the default setting.
  - \b hardwareHandshake - whether hardware handshaking is used or not.  HardwareHandshaking is off by default.

  The subsystem is supplied with small input and output buffers (of 100 characters each) and at present
  the implementation is interrupt per character so it's not particularly fast.

  \todo Convert to DMA interface for higher performance, and add support for debug UART
  */
typedef struct {
    AT91S_USART* uart;
    Queue* rxQueue;
    Queue* txQueue;

    Semaphore* rxSem;
    Semaphore* txSem;
    char rxBuf[2][SERIAL_RX_BUF_SIZE];
    int rxCurBuf;
    int rxBufCurPos;
    int rxBreak;
    char *rpr;

    int baud, bits, parity, stopBits, handshaking; 
} Serial_Internal;

// static stuff
void SerialIsr_Handler( int index );
extern Serial_Internal Serial_internals[SERIAL_PORTS];


//Serial( int channel, int q_size = 100 );
void Serial_open( int channel, int q_size );
void Serial_close( int channel );
void Serial_setBaud(int channel, int rate );
int Serial_baud( int channel );

void Serial_setDataBits(int channel, int bits );
int Serial_dataBits( int channel );

void Serial_setParity(int channel, int parity );
int Serial_parity( int channel );

void Serial_setStopBits( int channel, int bits );
int Serial_stopBits( int channel );

void Serial_setHandshaking( int channel, bool enable );
bool Serial_handshaking( int channel );

int Serial_writeChar( int channel, char character );
//int Serial_write( char* data, int length, int timeout = 0);
int Serial_write( int channel, char* data, int length, int timeout);
int Serial_writeDMA( int channel, void *data, int length, int timeout);
int Serial_bytesAvailable( int channel );
bool Serial_anyBytesAvailable( int channel );
//int Serial_read( char* data, int length, int timeout = 0);
int Serial_read( int channel, char* data, int length, int timeout );
//char Serial_readChar( int timeout = 0 );
char Serial_readChar( int channel, int timeout );
//int Serial_readDMA( char* data, int length, int timeout = 0 );
int Serial_readDMA( int channel, char* data, int length, int timeout );

void Serial_flush( int channel );
void Serial_clearErrors( int channel );
//bool Serial_errors( bool* overrun = 0, bool* frame = 0, bool* parity = 0 );
bool Serial_errors(  int channel, bool* overrun, bool* frame, bool* parity );
void Serial_startBreak( int channel );
void Serial_stopBreak( int channel );

//int _channel, _baud, bits, _parity, _stopBits, _handshaking;
void Serial_setDetails( int channel );


#endif // SERIAL_H
