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


#include "FreeRTOS.h"
#include "timer.h"
#include "debug/trace.h"

// debug
#include "utils/time.h"

#define MIN_DELAY_TICKS     2

//extern volatile portTickType xTickCount;

extern uint32_t _timer_tick_count;

//extern Timer_Manager timer_manager;
Timer_Manager *p_timer_manager;

void TimerIsr_Wrapper( void ) __attribute__ ((naked));
void Timer_Isr( );

void Timer_Isr( void )
{
    //Timer_Manager* manager = &timer_manager;
    Timer_Manager* manager = p_timer_manager;
    int status = manager->tc->TC_SR;
    if ( status & AT91C_TC_CPCS )
    {
        //TRACE_TMR(">>>Timer_Isr %d\r\n", xTickCount);
        TRACE_TMR(">>>Timer_Isr %d\r\n", Timer_tick_count_nonblock());
        int rc =  manager->tc->TC_RC;

        _timer_tick_count += rc / TIMER_CYCLES_PER_MS;

        manager->tc->TC_RC = 0xffff;
        manager->servicing = true;
        TIMER_DBG_PROCESSING(true);

        int jitter;
        manager->count++;
        jitter = manager->tc->TC_CV;

        manager->jitterTotal += jitter;
        if ( jitter > manager->jitterMax )
            manager->jitterMax = jitter;
        if ( jitter > manager->jitterMaxAllDay )
            manager->jitterMaxAllDay = jitter;

        // Run through once to make the callback calls
        Timer* timer = manager->first;
        manager->next = NULL;
        manager->previous = NULL;
        manager->nextTime = -1;

        int next_time_cv;

        while ( timer != NULL )
        {
            manager->next = timer->next;
            //timer->timeCurrent -= (manager->tc->TC_RC + manager->tc->TC_CV);
            //TRACE_TMR("timer %x %d (%d %d)\r\n", timer, timer->timeCurrent, manager->tc->TC_RC, manager->tc->TC_CV);
            
            int cv = manager->tc->TC_CV;
            timer->timeCurrent -= (rc + manager->tc->TC_CV);
            TRACE_TMR("timer %x %d (%d %d)\r\n", timer, timer->timeCurrent, rc, manager->tc->TC_CV);
            if ( timer->timeCurrent <= 0 )
            {
                //TRACE_TMR("tmr ex %d %d\r\n", timer->value, timeval - timer->started);
                //TRACE_INFO("tmrexp %d %d\r\n", timer->value, timeval - timer->started);
                if ( timer->repeat ) {
                    timer->timeCurrent += timer->timeInitial;
                    // debug
                    //timer->started = timeval;
                    timer->started = Timer_tick_count_nonblock();
                } else {
                    // remove it if necessary (do this first!)
                    if ( manager->previous == NULL )
                        manager->first = manager->next;
                    else
                        manager->previous->next = manager->next;     
                }

                if ( timer->callback != NULL )
                {
                    // in this callback, the callee is free to add and remove any members of this list
                    // which might effect the first, next and previous pointers
                    // so don't assume any of those local variables are good anymore
                    (*timer->callback)( timer->context );
                }

                // Assuming we're still on the list (if we were removed, then re-added, we'd be on the beggining of
                // the list with this task already performed) see whether our time is the next to run
                if ( !(( manager->previous == NULL && manager->first == timer ) ||
                    ( manager->previous != NULL && manager->previous->next == timer )) )
                {
                    if (timer->freeOnStop) {
                        // MV. right place here?
                        TRACE_TMR("timer %x freed (1)\r\n", timer);
                        Timer_closeStopped(timer);
                        //free(timer);

                        //timer = NULL;
                    }
                    timer = NULL;
                }
            } 
/*
            else
            {
                manager->previous = timer;
            }
*/
            if (timer) {
                manager->previous = timer;
                if ( manager->nextTime == -1 || timer->timeCurrent < manager->nextTime ) {
                    TRACE_TMR("nt %d\r\n", timer->timeCurrent);
                    manager->nextTime = timer->timeCurrent;
                    next_time_cv = cv;
                }
            }

            timer = manager->next;
        }

        if ( manager->first != NULL )
        {
#if 1 // MV
            // Add in whatever we're at now
            manager->nextTime += manager->tc->TC_CV;
#else
            TRACE_TMR("fix %d - (%d - %d)\r\n", manager->nextTime, manager->tc->TC_CV, next_time_cv);
            manager->nextTime -= (manager->tc->TC_CV - next_time_cv);

            if (manager->nextTime < 0) {
                TRACE_ERROR("timer: nextTime < 0\r\n");
                //panic("Timer_Isr");
                manager->nextTime = manager->tc->TC_CV + MIN_DELAY_TICKS;
            }
#endif
            // Make sure it's not too big
            if ( manager->nextTime > 0xFFFF )
                manager->nextTime = 0xFFFF;
            TRACE_TMR("tc_rc %d %d\r\n", manager->nextTime, manager->tc->TC_CV);

            //manager->tc->TC_CCR = AT91C_TC_CLKDIS;
            manager->tc->TC_RC = manager->nextTime;
            //Timer_enable();
        }
        else
        {
            manager->tc->TC_CCR = AT91C_TC_CLKDIS;
            manager->running = false;
        }

        jitter = manager->tc->TC_CV;
        manager->servicing = false;
        TIMER_DBG_PROCESSING(false);
        //TRACE_TMR("<<<Timer_Isr %d\r\n", xTickCount);
        TRACE_TMR("<<<Timer_Isr %d\r\n", Timer_tick_count_nonblock());
    }
    //unsigned int mask = 0x1 << manager->channel_id;
    //AT91C_BASE_AIC->AIC_ICCR = mask;

    AT91C_BASE_AIC->AIC_EOICR = 0; // Clear AIC to complete ISR processing
}

void TimerIsr_Wrapper( void )
{
    /* Save the context of the interrupted task. */
    portSAVE_CONTEXT();

    /* Call the handler to do the work.  This must be a separate
       function to ensure the stack frame is set up correctly. */
    Timer_Isr();

    /* Restore the context of whichever task will execute next. */
    portRESTORE_CONTEXT();
}

