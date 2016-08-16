#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>


typedef struct {
    uint16_t type;
    uint16_t width, height;
} bitmap_header;

typedef enum {
    BMP_8RGBA
} bitmap_type;

typedef struct {
    bitmap_header header;
} bitmap;

#endif // BITMAP_H
