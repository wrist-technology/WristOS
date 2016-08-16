#ifndef SAMPLE_H
#define SAMPLE_H

#include <types.h>

#define AUDIO_SAMPLE_DATA(p)  ((uint16_t*)((void*)(p) + sizeof(audio_sample)))

typedef struct {
    uint32_t sample_rate;
    uint32_t length;
    uint32_t loop;
} audio_sample;

#endif // SAMPLE_H
