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


#ifndef IO_H
#define IO_H

//#include "AT91SAM7X256.h"
#include "AT91SAM7SE512.h"
#include "types.h"

#define OUTPUT true
#define INPUT false
#define INVALID_PIN 1024 // a value too large for any pin

#define MAX_INTERRUPT_SOURCES 8

/**
  Control any of the 35 Input/Output signals on the Make Controller.

  Each of the signals on the Make Controller can be used as a digital in/out, and many
  can be used as part of one of the peripherals on the board (serial, usb, etc).  The Io 
  class allows you to easily control these signals.

  \section Usage
  The most common way to use the Io system is to control pins as GPIO (general purpose in/out).
  To do this, create a new Io object, specifying that you want it to be a GPIO and whether it should
  be an input or an output.  Check the list of signal names at \ref IoIndices.

  \code
  Io mysensor(IO_PA08, Io::GPIO, INPUT); // a new digital in on PA08, as a GPIO input
  bool isOn = mysensor.value(); // now read the input with the value() method
  \endcode

  You can just as easily set it up as a digital out, and turn it on and off:
  \code
  Io myLED(IO_PA08, Io::GPIO, OUTPUT); // a new digital in on PA08, as a GPIO output
  myLED.on(); // turn it on
  myLED.off(); // turn it off
// or do it with setValue if you want to keep the state in a variable
bool state = true;
myLED.setValue(state); // turn it on
state = false;
myLED.setValue(state); // turn it off

// we can even change it back into an input
myLED.setDirection(INPUT);
// hmm...the myLED name doesn't make so much sense now...oh well
\endcode

\section Interrupts
You can also register to get notified automatically, when the status on an Io
line changes, rather than constantly reading the value to see if it has changed.  
This can be much more efficient - see addInterruptHandler() and removeInterruptHandler() 
for details.

\section settings Additional Settings
Each Io pin also has some additional settings that can optionally be configured.
- \b Pullup - each signal has an optional pullup resistor that can be enabled.  To 
configure this, use the pullup() and setPullup() methods.
- <b>Glitch Filter</b> - each signal also has a glitch filter that can help reject 
short signals (most signals less than a master clock cycle, which is roughly 20 ns).
Check the filter() and setFilter() methods.

\ingroup io
*/
typedef enum { 
    IO_A,    /**< Peripheral A - refer to as Io::A */
    IO_B,    /**< Peripheral B - refer to as Io::B */
    IO_C,    /**< Peripheral C - refer to as Io::C */
    IO_GPIO  /**< General Purpose IO - refer to as Io::GPIO */
} Io_Peripheral;

typedef void (*handler) (void*);

typedef struct
{
    void* context;
    void (*handler)(void*);
    unsigned int mask;
    AT91S_PIO* port;
} Io_InterruptSource;

//static Io_InterruptSource isrSources[]; /// List of interrupt sources.
//static unsigned int isrSourceCount; /// Number of currently defined interrupt sources.

//static bool isrAInit;
//static bool isrBInit;

typedef struct
{
    /**
      Available options for peripheral config.
      Each pin can be configured as a general purpose input or output, or
      can be part of a peripheral (such as a serial port, etc.)
      */

    Io_Peripheral peripheral;
    unsigned int io_pin;
    AT91S_PIO* basePort;
    unsigned int mask;
    int InterruptSourceIndex;
} Io;

//Io *Io_new( int pin = INVALID_PIN, Peripheral = GPIO, bool output = OUTPUT );
Io *Io_new( int pin, Io_Peripheral peripheral, bool output );
void Io_init( Io *io, int pin, Io_Peripheral peripheral, bool output );
bool Io_valid( Io *io );

bool Io_setPin( Io *io, int pin );
int Io_pin( Io *io );

bool Io_value( Io *io );
bool Io_setValue( Io *io, bool onoff );
bool Io_on( Io *io );
bool Io_off( Io *io );

bool Io_direction( Io *io );
bool Io_setDirection( Io *io, bool output );

bool Io_pullup( Io *io );
bool Io_setPullup( Io *io, bool enabled );

bool Io_filter( Io *io );
bool Io_setFilter( Io *io, bool enabled );

int Io_peripheral( Io *io );
//bool Io_setPeripheral( Io *io, Peripheral periph, bool disableGpio = true );
bool Io_setPeripheral( Io *io, Io_Peripheral periph, bool disableGpio );
bool Io_releasePeripherals( Io *io );

//bool Io_addInterruptHandler( Io *io, handler h, void* context = 0 );
bool Io_addInterruptHandler( Io *io, handler h, void* context );
bool Io_removeInterruptHandler( Io *io );


void Io_initInterrupts(AT91S_PIO* pio, unsigned int priority);

void Io_Isr( AT91S_PIO* basePio );

/**
  \defgroup IoIndices IO Indices
  Indices (0-95) for each of the processor's IO lines.  PA0-PA31 are represented
  by indices 0 - 31, PB0-PB31 by indices 32 - 63, PC0-PC31 by indices 64 - 95.
  \ingroup Io
  @{
  */


#define IO_PA00  0 /**< IO 0, Port A */
#define IO_PA01  1 /**< IO 1, Port A */
#define IO_PA02  2 /**< IO 2, Port A */
#define IO_PA03  3 /**< IO 3, Port A */
#define IO_PA04  4 /**< IO 4, Port A */
#define IO_PA05  5 /**< IO 5, Port A */
#define IO_PA06  6 /**< IO 6, Port A */
#define IO_PA07  7 /**< IO 7, Port A */
#define IO_PA08  8 /**< IO 8, Port A */
#define IO_PA09  9 /**< IO 9, Port A */
#define IO_PA10 10 /**< IO 10, Port A */
#define IO_PA11 11 /**< IO 11, Port A */
#define IO_PA12 12 /**< IO 12, Port A */
#define IO_PA13 13 /**< IO 113, Port A */
#define IO_PA14 14 /**< IO 14, Port A */
#define IO_PA15 15 /**< IO 15, Port A */
#define IO_PA16 16 /**< IO 16, Port A */
#define IO_PA17 17 /**< IO 17, Port A */
#define IO_PA18 18 /**< IO 18, Port A */
#define IO_PA19 19 /**< IO 19, Port A */
#define IO_PA20 20 /**< IO 20, Port A */
#define IO_PA21 21 /**< IO 21, Port A */
#define IO_PA22 22 /**< IO 22, Port A */
#define IO_PA23 23 /**< IO 23, Port A */
#define IO_PA24 24 /**< IO 24, Port A */
#define IO_PA25 25 /**< IO 25, Port A */
#define IO_PA26 26 /**< IO 26, Port A */
#define IO_PA27 27 /**< IO 27, Port A */
#define IO_PA28 28 /**< IO 28, Port A */
#define IO_PA29 29 /**< IO 29, Port A */
#define IO_PA30 30 /**< IO 30, Port A */
#define IO_PA31 31 /**< IO 31, Port A */

#define IO_PB00 ( 32 +  0 ) /**< IO 0, Port B */
#define IO_PB01 ( 32 +  1 ) /**< IO 1, Port B */
#define IO_PB02 ( 32 +  2 ) /**< IO 2, Port B */
#define IO_PB03 ( 32 +  3 ) /**< IO 3, Port B */
#define IO_PB04 ( 32 +  4 ) /**< IO 4, Port B */
#define IO_PB05 ( 32 +  5 ) /**< IO 5, Port B */
#define IO_PB06 ( 32 +  6 ) /**< IO 6, Port B */
#define IO_PB07 ( 32 +  7 ) /**< IO 7, Port B */
#define IO_PB08 ( 32 +  8 ) /**< IO 8, Port B */
#define IO_PB09 ( 32 +  9 ) /**< IO 9, Port B */
#define IO_PB10 ( 32 + 10 ) /**< IO 10, Port B */
#define IO_PB11 ( 32 + 11 ) /**< IO 11, Port B */
#define IO_PB12 ( 32 + 12 ) /**< IO 12, Port B */
#define IO_PB13 ( 32 + 13 ) /**< IO 13, Port B */
#define IO_PB14 ( 32 + 14 ) /**< IO 14, Port B */
#define IO_PB15 ( 32 + 15 ) /**< IO 15, Port B */
#define IO_PB16 ( 32 + 16 ) /**< IO 16, Port B */
#define IO_PB17 ( 32 + 17 ) /**< IO 17, Port B */
#define IO_PB18 ( 32 + 18 ) /**< IO 18, Port B */
#define IO_PB19 ( 32 + 19 ) /**< IO 19, Port B */
#define IO_PB20 ( 32 + 20 ) /**< IO 20, Port B */
#define IO_PB21 ( 32 + 21 ) /**< IO 21, Port B */
#define IO_PB22 ( 32 + 22 ) /**< IO 22, Port B */
#define IO_PB23 ( 32 + 23 ) /**< IO 23, Port B */
#define IO_PB24 ( 32 + 24 ) /**< IO 24, Port B */
#define IO_PB25 ( 32 + 25 ) /**< IO 25, Port B */
#define IO_PB26 ( 32 + 26 ) /**< IO 26, Port B */
#define IO_PB27 ( 32 + 27 ) /**< IO 27, Port B */
#define IO_PB28 ( 32 + 28 ) /**< IO 28, Port B */
#define IO_PB29 ( 32 + 29 ) /**< IO 29, Port B */
#define IO_PB30 ( 32 + 30 ) /**< IO 30, Port B */
#define IO_PB31 ( 32 + 31 ) /**< IO 31, Port B */

#define IO_PC00 ( 64 +  0 ) /**< IO 0, Port C */
#define IO_PC01 ( 64 +  1 ) /**< IO 1, Port C */
#define IO_PC02 ( 64 +  2 ) /**< IO 2, Port C */
#define IO_PC03 ( 64 +  3 ) /**< IO 3, Port C */
#define IO_PC04 ( 64 +  4 ) /**< IO 4, Port C */
#define IO_PC05 ( 64 +  5 ) /**< IO 5, Port C */
#define IO_PC06 ( 64 +  6 ) /**< IO 6, Port C */
#define IO_PC07 ( 64 +  7 ) /**< IO 7, Port C */
#define IO_PC08 ( 64 +  8 ) /**< IO 8, Port C */
#define IO_PC09 ( 64 +  9 ) /**< IO 9, Port C */
#define IO_PC10 ( 64 + 10 ) /**< IO 10, Port C */
#define IO_PC11 ( 64 + 11 ) /**< IO 11, Port C */
#define IO_PC12 ( 64 + 12 ) /**< IO 12, Port C */
#define IO_PC13 ( 64 + 13 ) /**< IO 13, Port C */
#define IO_PC14 ( 64 + 14 ) /**< IO 14, Port C */
#define IO_PC15 ( 64 + 15 ) /**< IO 15, Port C */
#define IO_PC16 ( 64 + 16 ) /**< IO 16, Port C */
#define IO_PC17 ( 64 + 17 ) /**< IO 17, Port C */
#define IO_PC18 ( 64 + 18 ) /**< IO 18, Port C */
#define IO_PC19 ( 64 + 19 ) /**< IO 19, Port C */
#define IO_PC20 ( 64 + 20 ) /**< IO 20, Port C */
#define IO_PC21 ( 64 + 21 ) /**< IO 21, Port C */
#define IO_PC22 ( 64 + 22 ) /**< IO 22, Port C */
#define IO_PC23 ( 64 + 23 ) /**< IO 23, Port C */
#define IO_PC24 ( 64 + 24 ) /**< IO 24, Port C */
#define IO_PC25 ( 64 + 25 ) /**< IO 25, Port C */
#define IO_PC26 ( 64 + 26 ) /**< IO 26, Port C */
#define IO_PC27 ( 64 + 27 ) /**< IO 27, Port C */
#define IO_PC28 ( 64 + 28 ) /**< IO 28, Port C */
#define IO_PC29 ( 64 + 29 ) /**< IO 29, Port C */
#define IO_PC30 ( 64 + 30 ) /**< IO 30, Port C */
#define IO_PC31 ( 64 + 31 ) /**< IO 31, Port C */

/* @} */

/**
  \defgroup IoBits IO Bits
  The values to use to create a mask to pass into any of the \b Bits style functions.  
  Your mask values need to be of type longlong since it needs to represent 64 bits, 
  for the 64 IO lines.

  \b Example

  \code
  longlong mymask = 0;
  mymask |= (IO_PA00_BIT | IO_PA03_BIT | IO_PA11_BIT);
// now disable the pullups for lines PA00, PA03, and PA11
Io_PullupDisableBits( mymask );
\endcode
\ingroup Io
@{
*/

#define IO_PA00_BIT 1LL<<0x00 /**< IO 0, Port A */
#define IO_PA01_BIT 1LL<<0x01 /**< IO 1, Port A */
#define IO_PA02_BIT 1LL<<0x02 /**< IO 2, Port A */
#define IO_PA03_BIT 1LL<<0x03 /**< IO 3, Port A */
#define IO_PA04_BIT 1LL<<0x04 /**< IO 4, Port A */
#define IO_PA05_BIT 1LL<<0x05 /**< IO 5, Port A */
#define IO_PA06_BIT 1LL<<0x06 /**< IO 6, Port A */
#define IO_PA07_BIT 1LL<<0x07 /**< IO 7, Port A */
#define IO_PA08_BIT 1LL<<0x08 /**< IO 8, Port A */
#define IO_PA09_BIT 1LL<<0x09 /**< IO 9, Port A */
#define IO_PA10_BIT 1LL<<0x0A /**< IO 10, Port A */
#define IO_PA11_BIT 1LL<<0x0B /**< IO 11, Port A */
#define IO_PA12_BIT 1LL<<0x0C /**< IO 12, Port A */
#define IO_PA13_BIT 1LL<<0x0D /**< IO 13, Port A */
#define IO_PA14_BIT 1LL<<0x0E /**< IO 14, Port A */
#define IO_PA15_BIT 1LL<<0x0F /**< IO 15, Port A */
#define IO_PA16_BIT 1LL<<0x10 /**< IO 16, Port A */
#define IO_PA17_BIT 1LL<<0x11 /**< IO 17, Port A */
#define IO_PA18_BIT 1LL<<0x12 /**< IO 18, Port A */
#define IO_PA19_BIT 1LL<<0x13 /**< IO 19, Port A */
#define IO_PA20_BIT 1LL<<0x14 /**< IO 20, Port A */
#define IO_PA21_BIT 1LL<<0x15 /**< IO 21, Port A */
#define IO_PA22_BIT 1LL<<0x16 /**< IO 22, Port A */
#define IO_PA23_BIT 1LL<<0x17 /**< IO 23, Port A */
#define IO_PA24_BIT 1LL<<0x18 /**< IO 24, Port A */
#define IO_PA25_BIT 1LL<<0x19 /**< IO 25, Port A */
#define IO_PA26_BIT 1LL<<0x1A /**< IO 26, Port A */
#define IO_PA27_BIT 1LL<<0x1B /**< IO 27, Port A */
#define IO_PA28_BIT 1LL<<0x1C /**< IO 28, Port A */
#define IO_PA29_BIT 1LL<<0x1D /**< IO 29, Port A */
#define IO_PA30_BIT 1LL<<0x1E /**< IO 30, Port A */
#define IO_PA31_BIT 1LL<<0x1F /**< IO 31, Port A */

#define IO_PB00_BIT 1LL<<0x20 /**< IO 0, Port B */
#define IO_PB01_BIT 1LL<<0x21 /**< IO 1, Port B */
#define IO_PB02_BIT 1LL<<0x22 /**< IO 2, Port B */
#define IO_PB03_BIT 1LL<<0x23 /**< IO 3, Port B */
#define IO_PB04_BIT 1LL<<0x24 /**< IO 4, Port B */
#define IO_PB05_BIT 1LL<<0x25 /**< IO 5, Port B */
#define IO_PB06_BIT 1LL<<0x26 /**< IO 6, Port B */
#define IO_PB07_BIT 1LL<<0x27 /**< IO 7, Port B */
#define IO_PB08_BIT 1LL<<0x28 /**< IO 8, Port B */
#define IO_PB09_BIT 1LL<<0x29 /**< IO 9, Port B */
#define IO_PB10_BIT 1LL<<0x2A /**< IO 10, Port B */
#define IO_PB11_BIT 1LL<<0x2B /**< IO 11, Port B */
#define IO_PB12_BIT 1LL<<0x2C /**< IO 12, Port B */
#define IO_PB13_BIT 1LL<<0x2D /**< IO 13, Port B */
#define IO_PB14_BIT 1LL<<0x2E /**< IO 14, Port B */
#define IO_PB15_BIT 1LL<<0x2F /**< IO 15, Port B */
#define IO_PB16_BIT 1LL<<0x30 /**< IO 16, Port B */
#define IO_PB17_BIT 1LL<<0x31 /**< IO 17, Port B */
#define IO_PB18_BIT 1LL<<0x32 /**< IO 18, Port B */
#define IO_PB19_BIT 1LL<<0x33 /**< IO 19, Port B */
#define IO_PB20_BIT 1LL<<0x34 /**< IO 20, Port B */
#define IO_PB21_BIT 1LL<<0x35 /**< IO 21, Port B */
#define IO_PB22_BIT 1LL<<0x36 /**< IO 22, Port B */
#define IO_PB23_BIT 1LL<<0x37 /**< IO 23, Port B */
#define IO_PB24_BIT 1LL<<0x38 /**< IO 24, Port B */
#define IO_PB25_BIT 1LL<<0x39 /**< IO 25, Port B */
#define IO_PB26_BIT 1LL<<0x3A /**< IO 26, Port B */
#define IO_PB27_BIT 1LL<<0x3B /**< IO 27, Port B */
#define IO_PB28_BIT 1LL<<0x3C /**< IO 28, Port B */
#define IO_PB29_BIT 1LL<<0x3D /**< IO 29, Port B */
#define IO_PB30_BIT 1LL<<0x3E /**< IO 30, Port B */
#define IO_PB31_BIT 1LL<<0x3F /**< IO 31, Port B */

#define IO_PC00_BIT 1LL<<0x40 /**< IO 0, Port C */
#define IO_PC01_BIT 1LL<<0x41 /**< IO 1, Port C */
#define IO_PC02_BIT 1LL<<0x42 /**< IO 2, Port C */
#define IO_PC03_BIT 1LL<<0x43 /**< IO 3, Port C */
#define IO_PC04_BIT 1LL<<0x44 /**< IO 4, Port C */
#define IO_PC05_BIT 1LL<<0x45 /**< IO 5, Port C */
#define IO_PC06_BIT 1LL<<0x46 /**< IO 6, Port C */
#define IO_PC07_BIT 1LL<<0x47 /**< IO 7, Port C */
#define IO_PC08_BIT 1LL<<0x48 /**< IO 8, Port C */
#define IO_PC09_BIT 1LL<<0x49 /**< IO 9, Port C */
#define IO_PC10_BIT 1LL<<0x4A /**< IO 10, Port C */
#define IO_PC11_BIT 1LL<<0x4B /**< IO 11, Port C */
#define IO_PC12_BIT 1LL<<0x4C /**< IO 12, Port C */
#define IO_PC13_BIT 1LL<<0x4D /**< IO 13, Port C */
#define IO_PC14_BIT 1LL<<0x4E /**< IO 14, Port C */
#define IO_PC15_BIT 1LL<<0x4F /**< IO 15, Port C */
#define IO_PC16_BIT 1LL<<0x50 /**< IO 16, Port C */
#define IO_PC17_BIT 1LL<<0x51 /**< IO 17, Port C */
#define IO_PC18_BIT 1LL<<0x52 /**< IO 18, Port C */
#define IO_PC19_BIT 1LL<<0x53 /**< IO 19, Port C */
#define IO_PC20_BIT 1LL<<0x54 /**< IO 20, Port C */
#define IO_PC21_BIT 1LL<<0x55 /**< IO 21, Port C */
#define IO_PC22_BIT 1LL<<0x56 /**< IO 22, Port C */
#define IO_PC23_BIT 1LL<<0x57 /**< IO 23, Port C */
#define IO_PC24_BIT 1LL<<0x58 /**< IO 24, Port C */
#define IO_PC25_BIT 1LL<<0x59 /**< IO 25, Port C */
#define IO_PC26_BIT 1LL<<0x5A /**< IO 26, Port C */
#define IO_PC27_BIT 1LL<<0x5B /**< IO 27, Port C */
#define IO_PC28_BIT 1LL<<0x5C /**< IO 28, Port C */
#define IO_PC29_BIT 1LL<<0x5D /**< IO 29, Port C */
#define IO_PC30_BIT 1LL<<0x5E /**< IO 30, Port C */
#define IO_PC31_BIT 1LL<<0x5F /**< IO 31, Port C */

/** @} */

#endif
