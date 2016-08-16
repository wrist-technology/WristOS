#ifndef AUDIO_EVENT_H_
#define AUDIO_EVENT_H_

#include "types.h"

#define EVENT_AUDIO_STOP    1

typedef struct {
    uint16_t type;
    uint32_t data;
} event_data_audio;

#endif // AUDIO_EVENT_H_
