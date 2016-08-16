#include "battery.h"
#include "usb.h"
#include "debug/trace.h"
#include "event.h"
#include "task.h"
#include "queue.h"
#include "task_param.h"

#define WAKEUP_EVENT_CLOSE          1
#define WAKEUP_EVENT_TIMED_EVENT        2

static xTaskHandle battery_task_handle;
static gasgauge_stats _stats;
xQueueHandle battery_queue;

int battery_get_stats (gasgauge_stats *stats) {
    TRACE_INFO("battery_get_stats %x\r\n", stats);
    memcpy (stats, &_stats, sizeof(gasgauge_stats));
/*
    stats->voltage = _stats.voltage;
    stats->current = _stats.current;
    stats->state = _stats.state;
*/
    return 0;
}

#ifdef CFG_PM
void battery_timer_handler(void* context) {
    //TRACE_INFO("battery_timer_handler\r\n");

    portBASE_TYPE xHigherPriorityTaskWoken;
    uint8_t event = WAKEUP_EVENT_TIMED_EVENT;

    xQueueSendFromISR(battery_queue, &event, &xHigherPriorityTaskWoken);

    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}
#endif

static void battery_task( void* p ) {
    TRACE_INFO("battery task %x\r\n", xTaskGetCurrentTaskHandle());

#ifdef CFG_PM
    Timer timer;
    Timer_init(&timer, 0);
    Timer_setHandler(&timer, battery_timer_handler, NULL);
#endif

    while (true) {
        if(gasgauge_get_stats(&_stats)) {
            panic("battery_task");
        }
    
        if (_stats.voltage > 0) { 
            // TODO: BATTERY_STATE_CRITICAL
            // TODO: BATTERY_STATE_DISCHARGING

            if (_stats.state == GASGAUGE_STATE_NO_CHARGE) {
                if (_stats.voltage < 4050) {
                    gasgauge_charge(true);

                    event ev;
                    ev.type = EVENT_BATTERY;
                    ev.data.battery.state = BATTERY_STATE_CHARGING;
                    event_post(&ev);
                }
            } else if (_stats.state == GASGAUGE_STATE_CHARGED) {
                gasgauge_charge(false);

                event ev;
                ev.type = EVENT_BATTERY;
                ev.data.battery.state = BATTERY_STATE_CHARGED;
                event_post(&ev);
            }
        }
        //Task_sleep(10000);
        uint8_t battery_event;
#ifdef CFG_PM
        Timer_start(&timer, 10000, false, false);
        xQueueReceive(battery_queue, &battery_event, -1);
        Timer_stop(&timer);
#else
        xQueueReceive(battery_queue, &battery_event, 10000);
#endif
        if (battery_event == WAKEUP_EVENT_CLOSE) {
            break;
        } else if (battery_event == WAKEUP_EVENT_USB_DISCONNECTED) {
            event ev;
            ev.type = EVENT_BATTERY;
            ev.data.battery.state = BATTERY_STATE_DICHARGING;
            event_post(&ev);
        } else if (battery_event == WAKEUP_EVENT_USB_CONNECTED) {
        }
    }
#ifdef CFG_PM
    Timer_close(&timer);
#endif
    vQueueDelete(battery_queue);
    vTaskDelete(NULL);
}

int battery_init () {
    if (gasgauge_get_stats(&_stats)) {
        // no battery (probably)
        memset(&_stats, sizeof(_stats), 0);
        return -1;
    }

    battery_queue = xQueueCreate(1, sizeof(uint8_t));
    if (battery_queue == NULL) {
        panic("battery_init");
        return -1;
    }

    TRACE_INFO("battery_init()\r\n");

    // TODO: don't start the task if no battery (_stats.voltage == 0)
    if (xTaskCreate( battery_task, "battery", TASK_STACK_SIZE(TASK_BATTERY_STACK), NULL, TASK_BATTERY_PRI, &battery_task_handle ) != 1 ) {
        return -1;
    }

    return 0;
}

int battery_close() {
    uint8_t ev = WAKEUP_EVENT_CLOSE;
    xQueueSend(battery_queue, &ev, 0);
    return 0;
}
