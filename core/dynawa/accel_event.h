#ifndef ACCEL_EVENT_H_
#define ACCEL_EVENT_H_

#include "types.h"

typedef struct {
    uint32_t gesture;
    int16_t x, y, z;
} event_data_accel;

#endif // ACCEL_EVENT_H_
