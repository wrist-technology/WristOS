/*******************************************************************************

				(C) COPYRIGHT Cambridge Silicon Radio

FILE
				uSched.c

DESCRIPTION
				This module is micro Scheduler for Win32.

REVISION:		$Revision: 1.1.1.1 $ by $Author: ca01 $
*******************************************************************************/

#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/timeb.h>
#include "chw.h"
#include "uSched.h"
#include "debug/trace.h"
#include "bt.h"
#include "timer.h"

#define CFG_PM

//static HANDLE WakeUpEvent = NULL;
static xQueueHandle WakeUpEvent;
//CRITICAL_SECTION SchedulerMutex;
static xSemaphoreHandle SchedulerMutex;
static int terminationFlag;

/* Background Interrupts */

#define BG_INT_1_FLAG_MASK	0x0001
#define BG_INT_2_FLAG_MASK	0x0002

#define WAKEUP_EVENT_GENERIC        10
#define WAKEUP_EVENT_TIMED_EVENT    100
#define WAKEUP_EVENT_BG_INT1        1000
#define WAKEUP_EVENT_BG_INT2        1001

void (* bgIntFunction1)(void);
void (* bgIntFunction2)(void);
static unsigned short	bgIntFlags;

void register_bg_int(int bgIntNumber, void (* bgIntFunction)(void))
{
    TRACE_BT("register_bg_int %d\r\n", bgIntNumber);
    //EnterCriticalSection(&SchedulerMutex);
    xSemaphoreTake(SchedulerMutex, portMAX_DELAY);
	if (bgIntNumber == 1)
	{
		bgIntFunction1 = bgIntFunction;
	}
	else if (bgIntNumber == 2)
	{
		bgIntFunction2 = bgIntFunction;
	}
	else
	{
		//LeaveCriticalSection(&SchedulerMutex);
        xSemaphoreGive(SchedulerMutex);
/*
		fprintf(stderr, "PANIC: register_bg_int - unknown bgIntNumber (%d)!\n", bgIntNumber);
		exit(0);
*/
		TRACE_ERROR("PANIC: register_bg_int - unknown bgIntNumber (%d)!\n", bgIntNumber);
        panic("bgIntNumber");
	}
	//LeaveCriticalSection(&SchedulerMutex);
    xSemaphoreGive(SchedulerMutex);
}

void bg_int1(void)
{
    //TRACE_BT(">>>bg_int1 %x\r\n", xTaskGetCurrentTaskHandle());
    //EnterCriticalSection(&SchedulerMutex);
    xSemaphoreTake(SchedulerMutex, portMAX_DELAY);
	bgIntFlags |= BG_INT_1_FLAG_MASK;
	//LeaveCriticalSection(&SchedulerMutex);
    xSemaphoreGive(SchedulerMutex);
    //TRACE_BT("bg_int1 Queue %x\r\n", xTaskGetCurrentTaskHandle());
	//SetEvent(WakeUpEvent);
    uint16_t event = WAKEUP_EVENT_BG_INT1;
    //xQueueSend(WakeUpEvent, &event, portMAX_DELAY);
    xQueueSend(WakeUpEvent, &event, 0);
    //TRACE_BT("<<<bg_int1 %x\r\n", xTaskGetCurrentTaskHandle());
}

void bg_int2(void)
{
    //TRACE_BT(">>>bg_int2 %x\r\n", xTaskGetCurrentTaskHandle());
    //EnterCriticalSection(&SchedulerMutex);
    xSemaphoreTake(SchedulerMutex, portMAX_DELAY);
	bgIntFlags |= BG_INT_2_FLAG_MASK;
	//LeaveCriticalSection(&SchedulerMutex);
    xSemaphoreGive(SchedulerMutex);
    //TRACE_BT("bg_int2 Queue %x\r\n", xTaskGetCurrentTaskHandle());
	//SetEvent(WakeUpEvent);
    uint16_t event = WAKEUP_EVENT_BG_INT2;
    //xQueueSend(WakeUpEvent, &event, portMAX_DELAY);
    xQueueSend(WakeUpEvent, &event, 0);
    //TRACE_BT("<<<bg_int2 %x\r\n", xTaskGetCurrentTaskHandle());
}

void scheduler_wakeup(void)
{
    TRACE_BT("scheduler_wakeup\r\n");
    uint16_t event = WAKEUP_EVENT_GENERIC;
    xQueueSend(WakeUpEvent, &event, 0);
}

static void serviceBgInts(void)
{
    TRACE_BT("serviceBgInts\r\n");
	if (bgIntFlags & BG_INT_1_FLAG_MASK)
	{
		bgIntFlags &= ~BG_INT_1_FLAG_MASK;
	    //LeaveCriticalSection(&SchedulerMutex);
        xSemaphoreGive(SchedulerMutex);
		bgIntFunction1();
	    //EnterCriticalSection(&SchedulerMutex);
        xSemaphoreTake(SchedulerMutex, portMAX_DELAY);
	}
	if (bgIntFlags & BG_INT_2_FLAG_MASK)
	{
		bgIntFlags &= ~BG_INT_2_FLAG_MASK;
	    //LeaveCriticalSection(&SchedulerMutex);
        xSemaphoreGive(SchedulerMutex);
		bgIntFunction2();
	    //EnterCriticalSection(&SchedulerMutex);
        xSemaphoreTake(SchedulerMutex, portMAX_DELAY);
	}
}


/* Task handling */

static void (* uInitTask)(void);
static void (* uTask)(bt_command *cmd);
static int runTaskFlag = 0;

void ScheduleTaskToRun(void)
{
    TRACE_BT("ScheduleTaskToRun\r\n");
	runTaskFlag = 1;
}


/* Timers */

typedef struct TimedEventTag
{
    struct TimedEventTag	* next;
    TIME when;
    void (* eventFunction) (void);
} TimedEventType;

static TimedEventType * timedEvents = NULL;

TIME GetTime(void)
{
/*
    struct timeb tb;

    ftime(&tb);

    return ((TIME)((tb.time * 1000000) + (tb.millitm * 1000)));
*/
#ifdef CFG_PM
    return Timer_tick_count() * 1000;
#else
    return xTaskGetTickCount() * 1000; 
#endif
}

void * StartTimer(TIME delay, void (*fn) (void))
{
	TimedEventType * searchPointer;
	TIME when;
    TimedEventType * timedEvent = malloc(sizeof(TimedEventType));

	when = time_add(GetTime(), delay);
    //TRACE_BT("StartTimer %d %d\r\n", delay, when);
    timedEvent->when = when;
    timedEvent->eventFunction = fn;

    if (!timedEvents)
    {
        //TRACE_BT("head 1\r\n");
        timedEvent->next = NULL;
        timedEvents = timedEvent;
    }
    else
    {
        if (time_lt(when, timedEvents->when))
        {
            //TRACE_BT("head 2\r\n");
            timedEvent->next = timedEvents;
            timedEvents = timedEvent;
        }
        else
        {
            for (searchPointer = timedEvents; searchPointer; searchPointer = searchPointer->next)
            {
                //TRACE_BT("searchPointer %d\r\n", searchPointer->when);
                if (searchPointer->next) {
                    //TRACE_BT("next %d\r\n", searchPointer->next->when);
                }
                if (time_ge(when, searchPointer->when) && (!searchPointer->next || time_lt(when, searchPointer->next->when)))
                {
                    timedEvent->next = searchPointer->next;
                    searchPointer->next = timedEvent;
                    break;
                }
            }
        }
    }

    return ((void *) timedEvent); 
}

void StopTimer(void * timerHandle)
{
    TRACE_BT("StopTimer\r\n");
    TimedEventType * searchPointer;
    TimedEventType * previousPointer = NULL;

    for (searchPointer = timedEvents; searchPointer != NULL; searchPointer = searchPointer->next)
    {
        if (timerHandle == searchPointer)
        {
			if (previousPointer == NULL)
			{
				timedEvents = searchPointer->next;
			}
			else
			{
				previousPointer->next = searchPointer->next;
			}

            free(searchPointer);

            return;
        }

		previousPointer = searchPointer;
    }
} 


/* Scheduler */

void InitMicroSched(void (* initTask)(void), void (* task)(void))
{
    TRACE_BT("InitMicroSched\r\n");
	//InitializeCriticalSection(&SchedulerMutex);
    SchedulerMutex = xSemaphoreCreateMutex();
	//WakeUpEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    WakeUpEvent = xQueueCreate(1, sizeof(uint16_t));
	terminationFlag = 0;

	bgIntFunction1 = NULL;
	bgIntFunction2 = NULL;

	uInitTask = initTask;
	uTask = task;
}

#ifdef CFG_PM
void bt_sched_timer_handler(void* context) {
    //TRACE_INFO("bt_sched_timer_handler\r\n");

    portBASE_TYPE xHigherPriorityTaskWoken;
    uint16_t event = WAKEUP_EVENT_TIMED_EVENT;

    xQueueSendFromISR(WakeUpEvent, &event, &xHigherPriorityTaskWoken);

    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}
#endif

void MicroSched(void)
{
    TimedEventType * timedEvent;
	TIME now;
    bt_command bt_cmd;
    bt_command *bt_cmd_ptr = NULL;

#ifdef CFG_PM
    Timer timer;
    Timer_init(&timer, 0);
    Timer_setHandler(&timer, bt_sched_timer_handler, NULL);
#endif

    TRACE_BT("MicroSched\r\n");
	if (!terminationFlag && (uInitTask != NULL))
	{
		uInitTask();
	}

	while (!terminationFlag)
	{
	    //EnterCriticalSection(&SchedulerMutex);
        xSemaphoreTake(SchedulerMutex, portMAX_DELAY);
		if (bgIntFlags)
		{
			serviceBgInts();
		}
	    //LeaveCriticalSection(&SchedulerMutex);
        xSemaphoreGive(SchedulerMutex);

		if (runTaskFlag)
		{
			runTaskFlag = 0;
			uTask(bt_cmd_ptr);
            bt_cmd_ptr = NULL;
		}

        while (timedEvents)
        {
            if (!timedEvents)
            {
                break;
            }

            //TRACE_BT("timedEvents %d\r\n", timedEvents->when);
            if (time_gt(timedEvents->when, GetTime()))
            {
                break;
            }
            else
            {
			    //EnterCriticalSection(&SchedulerMutex);
                xSemaphoreTake(SchedulerMutex, portMAX_DELAY);
				if (bgIntFlags)
				{
					serviceBgInts();
				}
			    //LeaveCriticalSection(&SchedulerMutex);
                xSemaphoreGive(SchedulerMutex);

                timedEvent = timedEvents;
                timedEvents = timedEvent->next;

                timedEvent->eventFunction();

                free(timedEvent);
            }
        }

        if (!runTaskFlag)
        {
			if (!timedEvents)
            {
                //TRACE_BT("No timed events\r\n");
                //WaitForSingleObject(WakeUpEvent, INFINITE);
                uint16_t event;
                xQueueReceive(WakeUpEvent, &event, portMAX_DELAY); 
            }
            else
            {
                now = GetTime();
                //TRACE_BT("timed event %d %d\r\n", now, timedEvents->when);
                if(time_lt(now, timedEvents->when))
				{
                    //WaitForSingleObject(WakeUpEvent, time_sub(timedEvents->when, now) / 1000);
// TODO calc timeout in ticks
                    uint16_t event;
                    //TRACE_BT("wakeup event in %dms\r\n", time_sub(timedEvents->when, now) / 1000);


#ifdef CFG_PM
                    Timer_start(&timer, time_sub(timedEvents->when, now) / 1000, false, false);
                    xQueueReceive(WakeUpEvent, &event, -1); 
                    Timer_stop(&timer);
#else
                    xQueueReceive(WakeUpEvent, &event, time_sub(timedEvents->when, now) / 1000); 
#endif
				}
            }
            if (bt_get_command(&bt_cmd)) {
                bt_cmd_ptr = &bt_cmd;
                runTaskFlag = 1;
            }
        }
	}

	//CloseHandle(WakeUpEvent);
	//DeleteCriticalSection(&SchedulerMutex);

#ifdef CFG_PM
    Timer_close(&timer);
#endif
    // !!! TODO Purge timedEvents list
    while (timedEvents) {
        timedEvent = timedEvents;
        timedEvents = timedEvent->next;

        free(timedEvent);
    }
}

void CloseMicroSched(void) {
    TRACE_BT("CloseMicroSched\r\n");

    vQueueDelete(WakeUpEvent);
    vQueueDelete(SchedulerMutex);
    bt_stop_callback();
}

void TerminateMicroSched(void)
{
    TRACE_BT("TerminateMicroSched\r\n");
	terminationFlag = 1;
}
