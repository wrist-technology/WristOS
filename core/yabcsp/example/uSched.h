#ifndef U_SCHED_H__
#define U_SCHED_H__
/*******************************************************************************

				(C) COPYRIGHT Cambridge Silicon Radio

FILE
				uSched.h

DESCRIPTION:
				Header file for the micro scheduler

REVISION:		$Revision: 1.1.1.1 $ by $Author: ca01 $
*******************************************************************************/

#include "chw.h"

typedef uint32_t TIME;
#define time_add(t1, t2)    ((t1) + (t2))
#define time_lt(t1, t2) (time_sub((t1), (t2)) < 0)
#define time_gt(t1, t2) (time_sub((t1), (t2)) > 0)
#define time_ge(t1, t2) (time_sub((t1), (t2)) >= 0)
#define time_sub(t1, t2)    ((int32_t) (t1) - (int32_t) (t2))


extern TIME GetTime(void);

extern void bg_int1(void);
extern void bg_int2(void);
extern void register_bg_int(int bgIntNumber, void (* bgIntFunction)(void));

extern void ScheduleTaskToRun(void);

extern void * StartTimer(TIME delay, void (*fn) (void));
extern void StopTimer(void * timerHandle);

extern void InitMicroSched(void (* initTask)(void), void (* task)(void));
extern void MicroSched(void);
extern void TerminateMicroSched(void);

#endif /* U_SCHED_H__ */
