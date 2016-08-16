#include "event.h"
#include <FreeRTOS.h>
#include <semphr.h>
#include <debug/trace.h>

static xQueueHandle event_queue;

int event_init(int queue_length) {
    event_queue = xQueueCreate(queue_length, sizeof(event));
}

int event_post(event *ev) {
    if(xQueueSend(event_queue, ev, 0) != pdTRUE) {
        panic("event_post");
    }
}

int event_post_isr(event *ev) {
    portBASE_TYPE xHigherPriorityTaskWoken;
    if(xQueueSendFromISR(event_queue, ev, &xHigherPriorityTaskWoken) != pdTRUE) {
        panic("event_post_isr");
    }

    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

int event_get (event *ev, int timeout) {
    xQueueReceive(event_queue, ev, timeout);
}
