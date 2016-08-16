#ifndef USB_EVENT_H_
#define USB_EVENT_H_

#include "types.h";

#define EVENT_USB_CONNECTED        1
#define EVENT_USB_DISCONNECTED     2

typedef struct {
    uint8_t state;
} event_data_usb;

#endif // USB_EVENT_H_
