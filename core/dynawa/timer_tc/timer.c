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

#include "types.h"
#include "timer.h"
#include "error.h"
#include "rtos.h"
#include "irq_param.h"
#include "debug/trace.h"
// debug
#include "utils/time.h"

uint32_t Timer_tick_count();

static bool sync = true;
static bool timer_processing = false;

// statics
Timer_Manager timer_manager;
extern Timer_Manager *p_timer_manager;
// extern
void TimerIsr_Wrapper( );

uint32_t _timer_tick_count = 0;
uint32_t _timer_tick_count2 = 0;

static xSemaphoreHandle timer_mutex;
static xSemaphoreHandle tick_count_mutex;


/**
  Make a new timer.
  Note - the timer index selected will be used for all subsequent timers created.
  @param timer The hardware timer to use - valid options are 0, 1 and 2.  0 is the default.
  */
void Timer_init(Timer *timer, int timerindex)
{
    timer->magic = TIMER_MAGIC;

    if(!timer_manager.timer_count++)
        Timer_managerInit(timerindex);
}

void Timer_close(Timer *timer)
{
    Timer_stop(timer);
    if(--timer_manager.timer_count == 0)
        Timer_managerDeinit( );
}

void Timer_closeStopped(Timer *timer)
{
    if(--timer_manager.timer_count == 0)
        Timer_managerDeinit( );
}

/**
  Register a handler for this timer.
  Specify a handler function that should be called back at
  an interval specified in start().  If you have a handler registered with
  more than one timer, use the \b id to distinguish which timer is calling
  it at a given time.

  @param handler A function of the form \code void myHandler( int id ); \endcode
  @param id An id that will be passed into your handler, telling it which timer is calling it.
  */
void Timer_setHandler(Timer *timer, TimerHandler handler, void *context )
{
    timer->callback = handler;
    timer->context = context;
}

/**
  Start a timer.
  Specify if you'd like the timer to repeat and, if so, the interval at which 
  you'd like it to repeat.  If you have set up a handler with setHandler() then your 
  handler function will get called at the specified interval.  If the timer is already
  running, this will reset it.

  @param millis The number of milliseconds 
  @param repeat Whether or not to repeat - true by default.
  */
int Timer_start( Timer *timer, int millis, bool repeat, bool freeOnStop )
{
// debug
    if (millis < 0) {
        panic("Timer_start");
        return CONTROLLER_ERROR_ILLEGAL_PARAMETER_VALUE;
    }

    //timer->started = timeval;
    timer->started = Timer_tick_count();
    timer->value = millis;

    timer->timeCurrent = 0;
    timer->timeInitial = millis * TIMER_CYCLES_PER_MS;
    timer->repeat = repeat;
    timer->freeOnStop = freeOnStop;

    Timer *nextEntry = timer->next;
    timer->next = NULL;

    TRACE_TMR(">>Timer_start %x %d %d %d %d\r\n", timer, millis, repeat, freeOnStop, timer_manager.servicing);
    // this could be a lot smarter - for example, modifying the current period?
    if (sync && !timer_manager.servicing ) {
        Task_enterCritical();
        //xSemaphoreTake(timer_mutex, -1);
    }

    TIMER_DBG_PROCESSING(true);
    if ( !timer_manager.running )
    {
        //    Timer_SetActive( true );
        Timer_setTimeTarget( timer->timeInitial );
        Timer_enable();
    }  

/*
    // Calculate how long remaining
    int target = Timer_getTimeTarget();
    int timeCurrent = Timer_getTime();
    int remaining = target - timeCurrent;

    TRACE_TMR("t %d tc %d r %d\r\n", target, timeCurrent, remaining);
*/
    // Get the entry ready to roll
    timer->timeCurrent = timer->timeInitial;

    // Add entry
    Timer* first = timer_manager.first;
    timer_manager.first = timer;

    timer->next = first;


    // Are we actually servicing an interupt right now?
    if ( !timer_manager.servicing )
    {
        TRACE_TMR("srv 0\r\n");
        // No - so does the time requested by this new timer make the time need to come earlier?
        // first, unlink the timer 
        Timer *te = timer;
        while (te) {
            if (te->next && te->next == timer) {
                te->next = nextEntry;
                TRACE_TMR("Unlinking from the list %x\r\n", nextEntry);
                break;
            }
            te = te->next;
        }

        // Calculate how long remaining
        int target = Timer_getTimeTarget();
        int timeCurrent = Timer_getTime();
        int remaining = target - timeCurrent;

        TRACE_TMR("t %d tc %d r %d\r\n", target, timeCurrent, remaining);
        TRACE_TMR("%d < %d %d\r\n", timer->timeCurrent, remaining, TIMER_MARGIN);
        if ( timer->timeCurrent < ( remaining - TIMER_MARGIN ) )
        {
            // Damn it!  Reschedule the next callback
            Timer_setTimeTarget( target - ( remaining - timer->timeCurrent ));
            TRACE_TMR("%x rc %d cv %d\r\n", timer, timer_manager.tc->TC_RC, timer_manager.tc->TC_CV);
            // pretend that the existing time has been with us for the whole slice so that when the 
            // IRQ happens it credits the correct (reduced) time.
            timer->timeCurrent += timeCurrent;
        }
        else
        {
            // pretend that the existing time has been with us for the whole slice so that when the 
            // IRQ happens it credits the correct (reduced) time.
            timer->timeCurrent += timeCurrent;
            TRACE_TMR("sch %d\r\n", timer->timeCurrent);
        }
    }
    else
    {
        TRACE_TMR("srv 1\r\n");
        // Yep... we're servicing something right now

        // Make sure the previous pointer is OK.  This comes up if we were servicing the first item
        // and it subsequently wants to delete itself, it would need to alter the next pointer of the 
        // the new head... err... kind of a pain, this
        if ( timer_manager.previous == NULL )
            timer_manager.previous = timer;

        // Need to make sure that if this new time is the lowest yet, that the IRQ routine 
        // knows that.  Since we added this entry onto the beginning of the list, the IRQ
        // won't look at it again
        if ( timer_manager.nextTime == -1 || timer_manager.nextTime > timer->timeCurrent )
            timer_manager.nextTime = timer->timeCurrent;
    }

    TIMER_DBG_PROCESSING(false);
    if (sync && !timer_manager.servicing ) {
        Task_exitCritical();
        //xSemaphoreGive(timer_mutex);
    }

    TRACE_TMR("<<Timer_start %x\r\n", timer);
    return CONTROLLER_OK;
}

/** 
  Stop a timer.
  @return 0 on success.
  */
int Timer_stop( Timer *timer )
{
    TRACE_TMR(">>Timer_stop %x\r\n", timer);
    if (sync && !timer_manager.servicing ) {
        Task_enterCritical();
        //xSemaphoreTake(timer_mutex, -1);
    }
    TIMER_DBG_PROCESSING(true);

// MV TODO: reschedule timer?
    // Look through the running list - clobber the entry
    Timer* te = timer_manager.first;
    Timer* previousEntry = NULL;
    while ( te != NULL )
    {
        TRACE_TMR("timer2 %x\r\n", te);
        // check for the requested entry
        if ( te == timer )
        {
            TRACE_TMR("found %x %d\r\n", timer, timer_manager.servicing);
            if (te == te->next) {
                TRACE_ERROR("TIMER infinite loop %x %x\r\n", timer, te); 
                //while(1);
                panic("Timer_stop 1");
            }
            // remove the entry from the list
            if ( te == timer_manager.first ) {
                timer_manager.first = te->next;
            } else {
                previousEntry->next = te->next;
            }

            // make sure the in-IRQ pointers are all OK
            if ( timer_manager.servicing )
            {
                if ( timer_manager.previous == timer )
                    timer_manager.previous = previousEntry;
                if ( timer_manager.next == timer )
                    timer_manager.next = te->next;
            }

            // update the pointers - leave previous where it is
            te = te->next;

            if (timer->freeOnStop) {
                TRACE_TMR("timer %x freed (2)\r\n", timer);
                Timer_closeStopped(timer);

                if (timer->magic != TIMER_MAGIC) {
                    panic("Timer_stop 3");
                }
                timer->magic = 0;
                free(timer);
            }
        }
        else
        {
            previousEntry = te;
            te = te->next;

            if (te == te->next) {
                TRACE_ERROR("TIMER2 infinite loop %x %x\r\n", timer, te); 
                //while(1);
                panic("Timer_stop 2");
            }
        }
    }

    TIMER_DBG_PROCESSING(false);
    if (sync && !timer_manager.servicing ) {
        Task_exitCritical();
        //xSemaphoreGive(timer_mutex);
    }

    TRACE_TMR("<<Timer_stop %x\r\n", timer);
    return CONTROLLER_OK;
}

// Enable the timer.  Disable is performed by the ISR when timer is at an end
void Timer_enable( )
{
    // Enable the device
    // AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;
    timer_manager.tc->TC_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
    timer_manager.running = true;
}

int Timer_getTimeTarget( )
{
    return timer_manager.tc->TC_RC;
}

int Timer_getTime( )
{
    return timer_manager.tc->TC_CV;
}

void Timer_setTimeTarget( int target )
{
    TRACE_TMR("Timer_setTimeTarget %d\r\n", target);
    timer_manager.tc->TC_RC = ( target < 0xFFFF ) ? target : 0xFFFF;
}

int Timer_managerInit(int timerindex)
{
    tick_count_mutex = xSemaphoreCreateMutex();
    timer_mutex = xSemaphoreCreateMutex();
    if(timer_mutex == NULL) {
        return CONTROLLER_ERROR_INSUFFICIENT_RESOURCES;
    }
    
    p_timer_manager = &timer_manager;
    switch(timerindex)
    {
        case 1:
            timer_manager.tc = AT91C_BASE_TC1;
            timer_manager.channel_id = AT91C_ID_TC1;
            break;
        case 2:
            timer_manager.tc = AT91C_BASE_TC2;
            timer_manager.channel_id = AT91C_ID_TC2;
            break;
        default:
            timer_manager.tc = AT91C_BASE_TC0;
            timer_manager.channel_id = AT91C_ID_TC0;
/*
            timer_manager.tc = AT91C_BASE_RTTC;
            timer_manager.channel_id = AT91C_ID_SYS;
*/
            break;
    }

    timer_manager.first = NULL;
    timer_manager.count = 0;
    timer_manager.jitterTotal = 0;
    timer_manager.jitterMax = 0;  
    timer_manager.jitterMaxAllDay = 0;
    timer_manager.running = false;
    timer_manager.servicing = false;

    unsigned int mask = 0x1 << timer_manager.channel_id;
    AT91C_BASE_PMC->PMC_PCER = mask; // power up the selected channel

    // disable the interrupt, configure interrupt handler and re-enable
    AT91C_BASE_AIC->AIC_IDCR = mask;
    AT91C_BASE_AIC->AIC_SVR[ timer_manager.channel_id ] = (unsigned int)TimerIsr_Wrapper;
    AT91C_BASE_AIC->AIC_SMR[ timer_manager.channel_id ] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | IRQ_TIMER_PRI  ;
    AT91C_BASE_AIC->AIC_ICCR = mask;

    // Set the timer up.  We want just the basics, except when the timer compares 
    // with RC, retrigger
    //
    // MCK is 47923200
    // DIV1: A tick MCK/2 times a second
    // This makes every tick every 41.73344ns
    // DIV2: A tick MCK/8 times a second
    // This makes every tick every 167ns
    // DIV3: A tick MCK/32 times a second
    // This makes every tick every 668ns  
    // DIV4: A tick MCK/128 times a second
    // This makes every tick every 2.671us
    // DIV5: A tick MCK/1024 times a second
    // This makes every tick every 21.368us
    // CPCTRG makes the RC event reset the counter and trigger it to restart
    timer_manager.tc->TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK | AT91C_TC_CPCTRG;

    // Only really interested in interrupts when the RC happens
    timer_manager.tc->TC_IDR = 0xFF; 
    timer_manager.tc->TC_IER = AT91C_TC_CPCS; 

    // load the RC value with something
    timer_manager.tc->TC_RC = 0xFFFF;

    // Enable the interrupt
    AT91C_BASE_AIC->AIC_IECR = mask;

    return CONTROLLER_OK;
}

void Timer_managerDeinit( )
{
    //AT91C_BASE_AIC->AIC_IDCR = timer_manager.channel_id; // disable the interrupt
    //AT91C_BASE_PMC->PMC_PCDR = timer_manager.channel_id; // power down
    //MV
    unsigned int mask = 0x1 << timer_manager.channel_id;
    AT91C_BASE_AIC->AIC_IDCR = mask; // disable the interrupt
    AT91C_BASE_PMC->PMC_PCDR = mask; // power down

    vQueueDelete(timer_mutex);
    vQueueDelete(tick_count_mutex);
}

// MV
void Timer_setProcessingFlag(bool state)
{
    if (state && timer_processing) {
        TRACE_ERROR("TIMER REENTERED!!!\r\n");
        //while(1);
        panic("Timer_setProcessingFlag");
    }
    timer_processing = state;
}

//static uint32_t last_tick_count = 0;

uint32_t Timer_tick_count() {
#if 1
    return _timer_tick_count2;
#else
    unsigned int mask = 0x1 << timer_manager.channel_id;
    xSemaphoreTake(tick_count_mutex, -1);
    timer_manager.tc->TC_IDR = AT91C_TC_CPCS; 
    uint32_t ticks = _timer_tick_count + timer_manager.tc->TC_CV / TIMER_CYCLES_PER_MS;
    timer_manager.tc->TC_IER = AT91C_TC_CPCS; 
    xSemaphoreGive(tick_count_mutex);
    //TRACE_INFO("ticks: %x\r\n", ticks);
/*
    if (ticks < last_tick_count) {
        panic("Timer_tick_count");
    last_tick_count = ticks;
*/
    return ticks;
#endif
}

uint32_t Timer_tick_count_nonblock() {
#if 1
    return _timer_tick_count2;
#else
    unsigned int mask = 0x1 << timer_manager.channel_id;
    //xSemaphoreTake(tick_count_mutex, -1);
    timer_manager.tc->TC_IDR = AT91C_TC_CPCS; 
    uint32_t ticks = _timer_tick_count + timer_manager.tc->TC_CV / TIMER_CYCLES_PER_MS;
    timer_manager.tc->TC_IER = AT91C_TC_CPCS; 
    //xSemaphoreGive(tick_count_mutex);
    //TRACE_INFO("ticks: %x\r\n", ticks);
/*
    if (ticks < last_tick_count) {
        panic("Timer_tick_count_nonblock");
    last_tick_count = ticks;
*/
    return ticks;
#endif
}

uint32_t Timer_tick_count_nonblock2() {
#if 1
    return _timer_tick_count2;
#else
    unsigned int mask = 0x1 << timer_manager.channel_id;
    //xSemaphoreTake(tick_count_mutex, -1);
    taskENTER_CRITICAL();
    timer_manager.tc->TC_IDR = AT91C_TC_CPCS; 
    uint32_t ticks = _timer_tick_count + timer_manager.tc->TC_CV / TIMER_CYCLES_PER_MS;
    timer_manager.tc->TC_IER = AT91C_TC_CPCS; 
    taskEXIT_CRITICAL();
    //xSemaphoreGive(tick_count_mutex);
    //TRACE_INFO("ticks: %x\r\n", ticks);
/*
    if (ticks < last_tick_count) {
        panic("Timer_tick_count_nonblock");
    last_tick_count = ticks;
*/
    return ticks;
#endif
}

uint32_t Timer_tick_count_nonblock3() {
    uint32_t ticks = _timer_tick_count + timer_manager.tc->TC_CV / TIMER_CYCLES_PER_MS;
    return ticks;
}

void Timer_tick_count_sleep() {
    _timer_tick_count = 0;
}

uint32_t Timer_tick_count_wakeup(uint32_t delta) {
    //timer_manager.tc->TC_IDR = AT91C_TC_CPCS; 
    //uint32_t delta = _timer_tick_count + timer_manager.tc->TC_CV / TIMER_CYCLES_PER_MS;
    //timer_manager.tc->TC_IER = AT91C_TC_CPCS; 
    _timer_tick_count2 += delta;
    return delta;
}

void vApplicationTickHook( void ) {
    _timer_tick_count2++;
}
