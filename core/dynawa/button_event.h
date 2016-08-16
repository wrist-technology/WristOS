#ifndef BUTTON_EVENT_H
#define BUTTON_EVENT_H

#define EVENT_BUTTON_DOWN     1
#define EVENT_BUTTON_HOLD     2
#define EVENT_BUTTON_UP       3

#include <stdint.h>

typedef struct {
    uint8_t type;
    uint8_t id;
} event_data_button;

#endif // BUTTON_EVENT_H
