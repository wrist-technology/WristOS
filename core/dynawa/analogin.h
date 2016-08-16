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

#ifndef ANALOGIN_H
#define ANALOGIN_H

#include "rtos.h"

#define ANALOGIN_CHANNELS 8

/**
  10-bit analog inputs.
  The analog to digital converters read incoming signals from 0 - 3.3V.  They are rated as 5V tolerant, 
  but will not return meaningful values for anything above 3.3V.

  \section Values
  Analog inputs will return a value between 0 and 1023, corresponding to the range of \b 0 to \b 3.3V on the input.

  If you want to convert this to the actual voltage, you can use the following conversion:
  \code float voltage = 3.3 * ( ainValue / 1023.0 ) \endcode
  where \b ainValue is the AnalogIn value.

  A quicker version that doesn't use floating point, but will be slightly less precise:
  \code int voltage = ( 100 * ainValue ) / 1023 \endcode

  \ingroup io
  */
typedef struct {
    int index;
} AnalogIn;

void AnalogIn_init(AnalogIn *analogin, int channel);
void AnalogIn_close(AnalogIn *analogin);
int AnalogIn_value(AnalogIn *analogin);
int AnalogIn_valueWait(AnalogIn *analogin);

bool AnalogIn_multi(int values[]);

int AnalogIn_getIo( int index );
int AnalogIn_managerInit(void);
void AnalogIn_managerDeinit(void);

typedef struct {
    //Semaphore semaphore;
    xSemaphoreHandle semaphore;
    //Semaphore doneSemaphore;
    xSemaphoreHandle doneSemaphore;
    int activeChannels;
    bool waitingForMulti; // are we waiting for a multi conversion or just a single channel
    int multiConversionsComplete; // mask of which conversions have been completed
} AnalogIn_Manager;

///* OSC Interface */
//const char* AnalogInOsc_GetName( void );
//int AnalogInOsc_ReceiveMessage( int channel, char* message, int length );
//int AnalogInOsc_Async( int channel );

#endif
