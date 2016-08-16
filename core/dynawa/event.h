#ifndef EVENT_H
#define EVENT_H

#include <FreeRTOS.h>
#include <stdint.h>
#include <inttypes.h>
#include <button_event.h>
#include <timer_event.h>
#include <bt_event.h>
#include <battery_event.h>
#include <accel_event.h>
#include <audio_event.h>
#include <usb_event.h>

#define EVENT_WAIT_FOREVER  portMAX_DELAY

#define EVENT_BUTTON        1
#define EVENT_TIMER         2   
#define EVENT_BT            3   
#define EVENT_BATTERY       4   
#define EVENT_ACCEL         5   
#define EVENT_AUDIO         6   
#define EVENT_USB           7   

typedef struct {
    uint32_t type;
    union {
        event_data_button button;
        event_data_timer timer;
        bt_event bt;
        event_data_battery battery;
        event_data_accel accel;
        event_data_audio audio;
        event_data_usb usb;
    } data;
} event;

#endif // EVENT_H
