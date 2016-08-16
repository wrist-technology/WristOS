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

#include "appled.h"
//#include "AT91SAM7X256.h"
#include "AT91SAM7SE512.h"

#define APPLED_COUNT 4 

#if ( APPBOARD_VERSION == 50 )
  #define APPLED_0_IO IO_PB19
  #define APPLED_1_IO IO_PB20
  #define APPLED_2_IO IO_PB21
  #define APPLED_3_IO IO_PB22
#elif ( APPBOARD_VERSION == 90 )
  #define APPLED_0_IO IO_PA10
  #define APPLED_1_IO IO_PA11
  #define APPLED_2_IO IO_PA13
  #define APPLED_3_IO IO_PA15
#elif ( APPBOARD_VERSION == 95 || APPBOARD_VERSION == 100 )
  #define APPLED_0_IO IO_PA15
  #define APPLED_1_IO IO_PA13
  #define APPLED_2_IO IO_PA28
  #define APPLED_3_IO IO_PA27
#elif ( APPBOARD_VERSION == 200 )
  #define APPLED_0_IO IO_PA15
  #define APPLED_1_IO IO_PA13
  #define APPLED_2_IO IO_PA27
  #define APPLED_3_IO IO_PA28
#endif


Io* AppLed::leds[] = {0, 0, 0, 0};
//#ifdef OSC
//AppLedOSC* AppLed::oscHandler;
//#endif

/**
  Create a new AppLed
  @param index Which AppLed - valid options are 0, 1, 2, and 3

  \b Example
  \code
  AppLed a(1); // controlling AppLed 1
  a.setState(1); // turn it on
  a.setState(0); // turn it off
  \endcode
*/
AppLed::AppLed( int index )
{
  _index = index;
  if( index < 0 || index >= APPLED_COUNT )
    return;
  if( !leds[_index] )
    leds[_index] = new Io( getIo(_index), Io::GPIO, OUTPUT );
  setState(false);
//  #ifdef OSC
//  oscHandler = new AppLedOSC();
//  #endif
}

/**
  Turn an AppLed on or off.
  @param state True to turn it on, false to turn it off.

  \b Example
  \code
  AppLed* a = new AppLed(0); // allocate a new AppLed 0
  a->setState(1); // turn it on
  a->setState(0); // turn it off
  \endcode
*/
void AppLed::setState( bool state )
{
  leds[_index]->setValue( state );
}

/**
  Read whether an AppLed is currently on or not.

  @return true if it's on, false if it's not.

  \b Example
  \code
  AppLed appled(2);
  if( appled.getState() )
  {
    // then do this
  }
  else
  {
    // then do that
  }
  \endcode
*/
bool AppLed::getState( )
{
  return leds[_index]->value( );
}

int AppLed::getIo(int index)
{
  switch( index )
  {
    case 0:
      return APPLED_0_IO;
    case 1:
      return APPLED_1_IO;
    case 2:
      return APPLED_2_IO;
    case 3:
      return APPLED_3_IO;
    default:
      return -1;
  }
}

#ifdef OSC

/**
  \defgroup AppLEDOSC App LED - OSC
  Control the Application Board's Status LEDs via OSC.
	
	\section devices Devices
	There are 4 LEDs on the Make Application Board, numbered 0 - 3.
	
	\section properties Properties
	The LEDs have the following properties:
  - state
  - active

	\par State
	The \b state property corresponds to the on/off state of a given LED.
	For example, to turn on the first LED, send a message like
	\code /appled/0/state 1 \endcode
	To turn it off, send the message \code /appled/0/state 0 \endcode
	
	\par Active
	The \b active property corresponds to the active state of an LED.
	If an LED is set to be active, no other tasks will be able to
	write to it.  If you're not seeing appropriate
	responses to your messages to the LED, check the whether it's 
	locked by sending a message like
	\code /appled/0/active \endcode
	\par
	You can set the active flag by sending
	\code /appled/0/active 1 \endcode
*/

#include "osc.h"
#include "string.h"
#include "stdio.h"

const char* AppLedOSC::propertyList[] = {"state", 0};

/*
  We expect to get a message like /appled/index/property (data)
*/
int AppLedOSC::onNewMsg( OscTransport t, OscMessage* msg, int src_addr, int src_port )
{
  (void)src_addr;
  (void)src_port;
  int replies = 0;
  char* property = msg->addressElementAsString( 2 ); // address element 2 should be the property
  if( !property )
    return replies;
  
  int prop_index = propertyLookup( propertyList, property );
  if( prop_index < 0 ) // not one of our properties
    return replies;
  
  int index;
  OscRangeHelper rangeHelper(msg, 1, APPLED_COUNT); // element 1 should be our index
  while( rangeHelper.hasNext() )
  {
    index = rangeHelper.next();
    if( index < 0 || index >= APPLED_COUNT )
      continue;
    AppLed led(index);
    if( msg->data_count ) // setter...write the value to the led
    {
      switch( prop_index )
      {
        case 0: // state
          led.setState(msg->dataItemAsInt(0));
          break;
      }
    }
    else // getter...return a message
    {
      switch( prop_index )
      {
        case 0: // state
          Osc::get()->createMessage(t, msg->address, ",i", led.getState());
          replies++;
          break;
      }
    }
  }
  return replies;
}

int AppLedOSC::onQuery( OscTransport t, char* address, int element )
{
  int replies = 0;
  switch( element )
  {
    case 1: // index
    {
      int i;
      for(i = 0; i < APPLED_COUNT; i++)
      {
        Osc::get()->createMessage(t, address, ",i", i);
        replies++;
      }
      break;
    }
    case 2: // property
    {
      const char** prop = propertyList; // create message for each item in our propertyList
      while(prop)
      {
        Osc::get()->createMessage(t, address, ",s", *prop);
        replies++;
        prop++;
      }
      break;
    }
  }
  return replies;
}

#endif // OSC
