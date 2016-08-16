#ifndef TIMER_EVENT_H_
#define TIMER_EVENT_H_

#include "timer.h"

#define EVENT_TIMER_FIRED            1

typedef struct {
    uint8_t type;
    TimerHandle handle;
} event_data_timer;

#endif // TIMER_EVENT_H_
