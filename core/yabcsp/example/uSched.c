/*******************************************************************************

				(C) COPYRIGHT Cambridge Silicon Radio

FILE
				uSched.c

DESCRIPTION
				This module is micro Scheduler for Win32.

REVISION:		$Revision: 1.1.1.1 $ by $Author: ca01 $
*******************************************************************************/

#include <Windows.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/timeb.h>
#include "chw.h"
#include "uSched.h"


static HANDLE WakeUpEvent = NULL;
CRITICAL_SECTION SchedulerMutex;
static int terminationFlag;

/* Background Interrupts */

#define BG_INT_1_FLAG_MASK	0x0001
#define BG_INT_2_FLAG_MASK	0x0002

void (* bgIntFunction1)(void);
void (* bgIntFunction2)(void);
static unsigned short	bgIntFlags;

void register_bg_int(int bgIntNumber, void (* bgIntFunction)(void))
{
    EnterCriticalSection(&SchedulerMutex);
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
		LeaveCriticalSection(&SchedulerMutex);
		fprintf(stderr, "PANIC: register_bg_int - unknown bgIntNumber (%d)!\n", bgIntNumber);
		exit(0);
	}
	LeaveCriticalSection(&SchedulerMutex);
}

void bg_int1(void)
{
    EnterCriticalSection(&SchedulerMutex);
	bgIntFlags |= BG_INT_1_FLAG_MASK;
	LeaveCriticalSection(&SchedulerMutex);
	SetEvent(WakeUpEvent);
}

void bg_int2(void)
{
    EnterCriticalSection(&SchedulerMutex);
	bgIntFlags |= BG_INT_2_FLAG_MASK;
	LeaveCriticalSection(&SchedulerMutex);
	SetEvent(WakeUpEvent);
}

static void serviceBgInts(void)
{
	if (bgIntFlags & BG_INT_1_FLAG_MASK)
	{
		bgIntFlags &= ~BG_INT_1_FLAG_MASK;
	    LeaveCriticalSection(&SchedulerMutex);
		bgIntFunction1();
	    EnterCriticalSection(&SchedulerMutex);
	}
	if (bgIntFlags & BG_INT_2_FLAG_MASK)
	{
		bgIntFlags &= ~BG_INT_2_FLAG_MASK;
	    LeaveCriticalSection(&SchedulerMutex);
		bgIntFunction2();
	    EnterCriticalSection(&SchedulerMutex);
	}
}


/* Task handling */

static void (* uInitTask)(void);
static void (* uTask)(void);
static int runTaskFlag = 0;

void ScheduleTaskToRun(void)
{
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
    struct timeb tb;

    ftime(&tb);

    return ((TIME)((tb.time * 1000000) + (tb.millitm * 1000)));
}

void * StartTimer(TIME delay, void (*fn) (void))
{
	TimedEventType * searchPointer;
	TIME when;
    TimedEventType * timedEvent = malloc(sizeof(TimedEventType));

	when = time_add(GetTime(), delay);
    timedEvent->when = when;
    timedEvent->eventFunction = fn;

    if (!timedEvents)
    {
        timedEvent->next = NULL;
        timedEvents = timedEvent;
    }
    else
    {
        if (time_lt(when, timedEvents->when))
        {
            timedEvent->next = timedEvents;
            timedEvents = timedEvent;
        }
        else
        {
            for (searchPointer = timedEvents; searchPointer; searchPointer = searchPointer->next)
            {
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
	InitializeCriticalSection(&SchedulerMutex);
	WakeUpEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	terminationFlag = 0;

	bgIntFunction1 = NULL;
	bgIntFunction2 = NULL;

	uInitTask = initTask;
	uTask = task;
}


void MicroSched(void)
{
    TimedEventType * timedEvent;
	TIME now;

	if (!terminationFlag && (uInitTask != NULL))
	{
		uInitTask();
	}

	while (!terminationFlag)
	{
	    EnterCriticalSection(&SchedulerMutex);
		if (bgIntFlags)
		{
			serviceBgInts();
		}
	    LeaveCriticalSection(&SchedulerMutex);

		if (runTaskFlag)
		{
			runTaskFlag = 0;
			uTask();
		}

        while (timedEvents)
        {
            if (!timedEvents)
            {
                break;
            }

            if (time_gt(timedEvents->when, GetTime()))
            {
                break;
            }
            else
            {
			    EnterCriticalSection(&SchedulerMutex);
				if (bgIntFlags)
				{
					serviceBgInts();
				}
			    LeaveCriticalSection(&SchedulerMutex);

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
                WaitForSingleObject(WakeUpEvent, INFINITE);
            }
            else
            {
                now = GetTime();
                if(time_lt(now, timedEvents->when))
				{
                    WaitForSingleObject(WakeUpEvent, time_sub(timedEvents->when, now) / 1000);
				}
            }
        }
	}

	CloseHandle(WakeUpEvent);
	DeleteCriticalSection(&SchedulerMutex);
}

void TerminateMicroSched(void)
{
	terminationFlag = 1;
}
