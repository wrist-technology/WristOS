#ifndef BUTTON_H
#define BUTTON_H

#include "io.h"
#include "timer.h"

#define NUM_BUTTONS     5

typedef struct {
    Io io;
    bool down;
    bool held;
    Timer timer;
    bool timer_started;
} Button;

#endif // BUTTON_H
