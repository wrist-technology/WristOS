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

#include "stdlib.h"
#include "board.h"
//#include "AT91SAM7X256.h"
#include "AT91SAM7SE512.h"
#include "io.h"
#include "core.h"
#include "irq_param.h"
#include "debug/trace.h"

#define IO_PIN_COUNT 96
#define MAX_INTERRUPT_SOURCES 8

// statics
Io_InterruptSource Io_isrSources[MAX_INTERRUPT_SOURCES];
unsigned int Io_isrSourceCount = 0;
static bool Io_isrAInit = false;
static bool Io_isrBInit = false;
static bool Io_isrCInit = false;

extern void IoAIsr_Wrapper( );
extern void IoBIsr_Wrapper( );
extern void IoCIsr_Wrapper( );

/**
  Create a new Io object.

  If you're configuring it as a GPIO, you can use the \b INPUT and \b OUTPUT 
  symbols instead of true/false, for convenience.

  @param index Which pin to control - see IoIndices
  @param peripheral (optional) Which Peripheral to configure the Io as - defaults to GPIO.
  @param output (optional) If peripheral is GPIO, set whether it's an input or an output - 
  defaults to OUTPUT.

  \b Example
  \code
  Io io(IO_PA07); // control pin PA07, defaults to GPIO configured as an output
  io.on(); // turn the output on

// or specify more config info
Io io(IO_PA08, Io::GPIO, INPUT); // control pin PA08, as an input
bool is_pa08_on = io.value(); // is it on?
\endcode
*/
void Io_init( Io *io,  int index, Io_Peripheral peripheral, bool output  )
{

    io->io_pin = INVALID_PIN;  
    if( !Io_setPin( io, index ) ) {
        return;
    }
    Io_setPeripheral( io, peripheral, true );
    if( peripheral == IO_GPIO )
        Io_setDirection( io, output );
    io->InterruptSourceIndex = -1;
}

Io *Io_new( int index, Io_Peripheral peripheral, bool output  )
{
    Io *io = (Io*)malloc (sizeof(Io));
    if (!io)
        return NULL;

    Io_init(io, index, peripheral, output);

    return io;
}

bool Io_valid( Io *io )
{
    return io->io_pin != INVALID_PIN;
}

/**
  Set which pin an Io is controlling.

  See \ref IoIndices for the list of possible values.

  \b Example
  \code
  Io io; // nothing specified - invalid until configured
  io.setPin(IO_PA27); // set it to control PA27
  \endcode
  */
bool Io_setPin( Io *io, int pin )
{
    if ( pin < 0 || pin > IO_PIN_COUNT )
        return false;
    io->io_pin = pin;
    if( io->io_pin < 32 )
    {
        io->basePort = AT91C_BASE_PIOA;
        io->mask = 1 << io->io_pin;
    }
    else if (io->io_pin < 64 )
    {
        io->basePort = AT91C_BASE_PIOB;
        io->mask = 1 << ( io->io_pin & 0x1F );
    }
    else
    {
        io->basePort = AT91C_BASE_PIOC;
        io->mask = 1 << ( io->io_pin & 0x1F );
    }
    return true;
}

/**
  Read which pin an Io is controlling.

  @return The pin being controlled - see \ref IoIndices for a list of possible values.

  \b Example
  \code
  Io io(IO_PA09);
  int p = io.pin();
  p == IO_PA09; // true
  \endcode
  */
int Io_pin( Io *io )
{
    return ( io->io_pin == INVALID_PIN ) ? -1 : io->io_pin;
}

/**
  Read whether an Io is on or off.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b input.
  @return True if the pin is high, false if it's low.

  \b Example
  \code
  Io io(IO_PB28, Io::GPIO, false); // new io, configured as a GPIO and an input
  bool is_pb28_on = io.value(); // is it on?
  \endcode
  */
bool Io_value( Io *io )
{
    return ( ( io->basePort->PIO_PDSR & io->mask ) != 0 );
}

/**
  Turn an Io on or off.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.
  If you want to turn the Io on or off directly, use on() or off().
  @param onoff True to turn it on, false to turn it off.
  @return True on success, false on failure.

  \b Example
  \code
  Io io(IO_PB28); // new io, defaults to GPIO output
  io.setValue(true); // turn it on
  \endcode
  */
bool Io_setValue( Io *io, bool onoff )
{
    if ( onoff )
        io->basePort->PIO_SODR = io->mask; // set it
    else
        io->basePort->PIO_CODR = io->mask; // clear
    return true;
}

/**
  Turn on Io on.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.

  This is slightly faster than setValue() since it doesn't have to check
  whether to turn it on or off.
  @return True on success, false on failure.

  \b Example
  \code
  Io io(IO_PB18); // defaults to GPIO output
  io.on(); // turn it on
  \endcode
  */
bool Io_on( Io *io )
{
    io->basePort->PIO_SODR = io->mask;
    return true;
}

/**
  Turn an Io off.
  For this to be meaningful, the Io must be configured as a \b GPIO and set as an \b output.

  This is slightly faster than setValue() since it doesn't have to check
  whether to turn it on or off.
  @return True on success, false on failure.

  \b Example
  \code
  Io io(IO_PB18); // defaults to GPIO output
  io.off(); // turn it off
  \endcode
  */
bool Io_off( Io *io )
{
    io->basePort->PIO_CODR = io->mask;
    return true;
}

/**
  Read whether an Io is an input or an output.
  For this to be meaningful, the Io must be configured as a \b GPIO.
  @return True if it's an output, false if it's an input.

  \b Example
  \code
  Io io(IO_PA13);
  if(io.direction())
  {
// then we're an output
}
else
{
// we're an input
}
\endcode
*/
bool Io_direction( Io *io )
{
    return ( io->basePort->PIO_OSR & io->mask ) != 0;
}

/**
  Set an Io as an input or an output
  For this to be meaningful, the Io must be configured as a \b GPIO.

  You can also use the \b OUTPUT and \b INPUT symbols instead of true/false, 
  for convenience.
  @param output True to set it as an output, false to set it as an input.

  \b Example
  \code
  Io io(IO_PA01); // is a GPIO output by default
  io.setDirection(INPUT); // now change it to an input
  \endcode
  */
bool Io_setDirection( Io *io, bool output )
{
    if( output ) // set as output
        io->basePort->PIO_OER = io->mask;
    else // set as input
        io->basePort->PIO_ODR = io->mask;
    return true;
}

/**
  Read whether the pullup is enabled for an Io.
  @return True if pullup is enabled, false if not.

  \b Example
  \code
  Io io(IO_PA14);
  bool is_pa14_pulledup = io.pullup(); // is it pulled up?
  \endcode
  */
bool Io_pullup( Io *io )
{
    return ( io->basePort->PIO_PPUSR & io->mask ) == 0; // The PullUp status register is inverted.
}

/**
  Set whether the pullup is enabled for an Io.
  @param enabled True to enabled the pullup, false to disable it.

  \b Example
  \code
  Io pa14(IO_PA14);
  pa14.setPullup(true); // turn the pullup on
  \endcode
  */
bool Io_setPullup( Io *io, bool enabled )
{
    if( enabled ) // turn it on
        io->basePort->PIO_PPUER = io->mask;
    else // turn it off
        io->basePort->PIO_PPUDR = io->mask;
    return true;
}

/**
  Read whether the glitch filter is enabled for an Io.
  @return True if the filter is enabled, false if not.

  \b Example
  \code
  Io pa8(IO_PA08);
  bool is_pa8_filtered = pa8.filter(); // is the filter on?
  \endcode
  */
bool Io_filter( Io *io )
{
    return io->basePort->PIO_IFSR & io->mask;
}

/**
  Set whether the glitch filter is enabled for an Io.
  @param enabled True to enable the filter, false to disable it.

  \b Example
  \code
  Io io(IO_PB12);
  io.setFilter(true); // turn the filter on
  \endcode
  */
bool Io_setFilter( Io *io, bool enabled )
{
    if( enabled ) // turn it on
        io->basePort->PIO_IFER = io->mask;
    else // turn it off
        io->basePort->PIO_IFDR = io->mask;
    return true;
}

/*
   Read which perihperal an Io has been configured as.

   \b Example
   \code

   \endcode
   */
// int Io::peripheral( )
// {
//   if ( io_pin == INVALID_PIN )
//     return 0;
//     
//   if ( io_pin < 32 ) 
//     return ( AT91C_BASE_PIOA->PIO_PSR & ( 1 << io_pin ) ) != 0;
//   else
//     return ( AT91C_BASE_PIOB->PIO_PSR & ( 1 << ( io_pin & 0x1F ) ) ) != 0;
// }

/**
  Set the peripheral an Io should be configured as.

  @param periph The Peripheral to set this pin to.
  @param disableGpio (optional) If you're specifying peripheral A or B, this can
  optionally make sure the GPIO configuration is turned off.

  \b Example
  \code
  Io io(IO_PB09); // defaults to GPIO output
  io.setPeripheral(Io::A); // set to A & disable GPIO
  \endcode
  */
bool Io_setPeripheral( Io *io, Io_Peripheral periph, bool disableGpio )
{
    switch( periph )
    {
        case IO_A: // disable pio for each
            if( disableGpio )
                io->basePort->PIO_PDR = io->mask;
            io->basePort->PIO_ASR = io->mask;
            break;
        case IO_B:
            if( disableGpio )
                io->basePort->PIO_PDR = io->mask;
            io->basePort->PIO_BSR = io->mask;
            break;
        case IO_C:
            if( disableGpio )
                io->basePort->PIO_PDR = io->mask;
            io->basePort->PIO_BSR = io->mask;
            break;
        case IO_GPIO:
            io->basePort->PIO_PER = io->mask;
            break;
    }
    return true;
}

// bool Io::releasePeripherals( )
// {
//   if ( io_pin == INVALID_PIN )
//     return 0;
//   
//   // check each possible peripheral, and make sure it's cleared
//   return false;
// }

/**
  Add an interrupt handler for this signal.
  If you want to get notified when the signal on this pin changes,
  you can register an interrupt handler, instead of constantly reading the pin.
  To do this, define a function you want to be called when the pin changes, and
  then register it with the Io you want to monitor.  The function has to have a
  specific signature - it must be of the form:
  \code void myHandler(void* context) \endcode

  This function will be called any time the signal on the Io pin changes.  When you 
  register the handler, you can provide a pointer to some context that will be passed
  into your handler.  This can be an instance of a class, if you want to 

  \b Example
  \code
  void myHandler(void* context); // declare our handler

  void myTask(void* p)
  {
  Io io(IO_PB27, Io::GPIO, INPUT);   // Io on AnalogIn 0 - as digital input
  io.addInterruptHandler(myHandler); // register our handler

  while(true)
  {
// do the rest of my task...
}
}

int count = 0; // how many times has our interrupt triggered?
// now this will get called every time the value on PB27 changes
void myHandler(void* context)
{
count++;
if(count > 100)
count = 0;
}
\endcode

@param h The function to be called when there's an interrupt
@param context (optional) Context that will be passed into your handler, if desired.
@return True if the handler was registered successfully, false if not.
*/
bool Io_addInterruptHandler(Io *io, handler h, void* context)
{
    int i;
    for(i = 0; i < MAX_INTERRUPT_SOURCES; i++) {
        if (Io_isrSources[i].handler == NULL) {
            break;
        }
    }
    if(i == MAX_INTERRUPT_SOURCES) {
        panic("Io_addInterruptHandler");
        return false;
    }

    Io_isrSources[i].port = io->basePort;
    Io_isrSources[i].mask = io->mask;

    // if this is the first time for either channel, set it up
    if(     (!Io_isrAInit && io->basePort == AT91C_BASE_PIOA) || 
            (!Io_isrBInit && io->basePort == AT91C_BASE_PIOB) || 
            (!Io_isrCInit && io->basePort == AT91C_BASE_PIOC) )
    {
/*
#define AT91C_AIC_SRCTYPE     (0x3 <<  5) // (AIC) Interrupt Source Type
#define     AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL       (0x0 <<  5) // (AIC) Internal Sources Code Label High-level Sensitive
#define     AT91C_AIC_SRCTYPE_EXT_LOW_LEVEL        (0x0 <<  5) // (AIC) External Sources Code Label Low-level Sensitive
#define     AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE    (0x1 <<  5) // (AIC) Internal Sources Code Label Positive Edge triggered
#define     AT91C_AIC_SRCTYPE_EXT_NEGATIVE_EDGE    (0x1 <<  5) // (AIC) External Sources Code Label Negative Edge triggered
#define     AT91C_AIC_SRCTYPE_HIGH_LEVEL           (0x2 <<  5) // (AIC) Internal Or External Sources Code Label High-level Sensitive
#define     AT91C_AIC_SRCTYPE_POSITIVE_EDGE        (0x3 <<  5) // (AIC) Internal Or External Sources Code Label Positive Edge triggered
*/
        Io_initInterrupts(io->basePort, (AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | IRQ_IO_PRI) );
        // MV
        //Io_initInterrupts(io->basePort, (AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE | IRQ_IO_PRI) );
    }

    io->basePort->PIO_ISR;                                   // clear the status register
    io->basePort->PIO_IER = Io_isrSources[i].mask; // enable our channel

    Io_isrSources[i].handler = h;
    Io_isrSources[i].context = context;
    
    io->InterruptSourceIndex = i;
    Io_isrSourceCount++;

    return true;
}

/**
  Disable the interrupt handler for this io line.
  If you've already registered an interrupt with addInterruptHandler()
  this will disable it.

  \b Example
  \code
  Io io(IO_PB27, Io::GPIO, INPUT);
  io.addInterruptHandler(myHandler); // start notifications

// ... some time later, we want to stop getting notified

io.removeInterruptHandler( ); // stop notifications
\endcode

@return True on success, false on failure.
*/
bool Io_removeInterruptHandler( Io *io )
{
    if (io->InterruptSourceIndex == -1) {
        return true;
    }
    io->basePort->PIO_IDR = io->mask;
    Io_isrSources[io->InterruptSourceIndex].handler = NULL; 
    io->InterruptSourceIndex = -1;
    Io_isrSourceCount--;
    return true;
}

/*
   Turn on interrupts for a pio channel - a, b or c
   at a given priority.
   */
void Io_initInterrupts(AT91S_PIO* pio, unsigned int priority)
{
    unsigned int chan;
    void (*isr_handler)( );

    if( pio == AT91C_BASE_PIOA )
    {
        chan = AT91C_ID_PIOA;
        isr_handler = IoAIsr_Wrapper;
        Io_isrAInit = true;
    }
    else if( pio == AT91C_BASE_PIOB )
    {
        chan = AT91C_ID_PIOB;
        isr_handler = IoBIsr_Wrapper;
        Io_isrBInit = true;
    }
    else if( pio == AT91C_BASE_PIOC )
    {
        chan = AT91C_ID_PIOC;
        isr_handler = IoCIsr_Wrapper;
        Io_isrCInit = true;
    }
    else
        return;

    pio->PIO_ISR;                         // clear with a read
    pio->PIO_IDR = 0xFFFFFFFF;            // disable all by default
    AIC_ConfigureIT(chan, priority, isr_handler); // set it up
    AT91C_BASE_AIC->AIC_IECR = 1 << chan;     // enable it
}



