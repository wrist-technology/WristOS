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

#ifndef TIMER_H
#define TIMER_H

//#include "AT91SAM7X256.h"
#include "AT91SAM7SE512.h"
#include "types.h"

#define TIMER_DBG_PROCESSING(s)     Timer_setProcessingFlag(s)
#define TIMER_COUNT 8
#define TIMER_MARGIN 2

#define TIMER_MAGIC 0x11223344

#define TIMER_CYCLES_PER_MS 47
#define MIN_DELAY_TICKS     2

typedef void (*TimerHandler)( void *context ); /**< A handler for timers. */

/**
  Provides a timer in a millisecond timeframe.
  
  A Timer is a convenient way to schedule recurring events.  

  \section typicalusage Typical Usage
  A timer can be set up to call a function of yours at a regular interval.  Below is a simple
  example that will increment a counter and take some action when it gets above a certain amount.  

  \code
  // first, we'll create a function that will get called by the timer.
  void myHandler( int id );
  int count = 0; // our current count
  void myHandler( int id )
  {
    count++;
    if(count > 500 )
    {
      // then do something here
      count = 0; // and reset the count
    }
  }
  
  // now create the timer
  Timer t; // don't need to specify a channel - defaults to 0
  t.setHandler( myHandler, 34 ); // register our handler with this timer.
  t.start( 250 ); // call our handler every 250 milliseconds
  \endcode
  
  \section notes Notes
  A few things to be aware of when using Timers:
  - In your handler, you must not sleep or make any calls that will take a long time.  You may, however, use
  the Queue and Semaphore calls that end in \b fromISR in order to synchronize with running tasks.
  - To modify an existing Timer, stop() it and then start() it again.  Modifying it while running is not recommended.
  - There are 3 identical hardware timers on the Make Controller.  The first Timer that you create
  will specify which of them to use, and it will be used for all subsequent timers created.  
  If you don't specify a channel, 0 is used which is usually fine.  Specifically, the \ref FastTimer is on 
  channel 2 by default, so make sure to keep them separate if you're using them at the same time.
  
  For higher resolution timing, check the \ref FastTimer
*/

struct Timer;

typedef struct
{
    void *context;
    int timeInitial;
    //int timeCurrent;
    unsigned int target;
    bool repeat;
    bool freeOnStop;
    TimerHandler callback;
    struct Timer* next;
// debug
    uint32_t started;
    uint32_t value;
    uint32_t magic;
} Timer;

typedef Timer* TimerHandle;

//void Timer_init( Timer *timer, int timer = 0);
void Timer_init( Timer *timer, int timerindex);
void Timer_close ( Timer *timer );
void Timer_setHandler( Timer *timer, TimerHandler handler, void *context );
//int Timer_start( Timer *timer, int millis, bool repeat = true );
int Timer_start( Timer *timer, int millis, bool repeat, bool freeOnStop );
int Timer_stop( Timer *timer );
int Timer_getTimeTarget( );
int Timer_getTime( );
void Timer_setTimeTarget( unsigned int target );
int Timer_managerInit( int timerindex);
void Timer_managerDeinit( );
void Timer_enable( );

typedef struct
{
    unsigned int timer_count; // how many timers have been created?
    unsigned int channel_id;
    short count;
  
    int jitterTotal;
    int jitterMax;
    int jitterMaxAllDay;
  
    char running;
    char servicing;
  
    unsigned int nextTime;
    bool nextTimeSet;
    int temp;
    
    Timer* first;
    Timer* next;
    Timer* previous;
    Timer* lastAdded;
    AT91S_RTTC* tc;
} Timer_Manager;

#endif
