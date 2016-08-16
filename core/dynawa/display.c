#include "types.h"
#include "debug/trace.h"
#include "task_param.h"
#include "rtos.h"
#include "analogin.h"
#include "timer.h"

#define WAKEUP_EVENT_CLOSE          1
#define WAKEUP_EVENT_TIMED_EVENT    10
#define WAKEUP_EVENT_START          20
#define WAKEUP_EVENT_STOP           21

static bool display_power_state = false;
static uint8_t display_brightness_level = 2;

static Task display_brightness_task_handle;
static bool display_brightness_task_run = false;

static xQueueHandle display_queue;

int display_brightness(uint8_t level);

int display_power(bool on) {

    TRACE_INFO("display_power(%d)\r\n", on);

    if (display_power_state == on) {
        return 0;
    }

    if (on) {
        display_brightness(display_brightness_level);
        oled_screen_state(true);
    } else {
        if (!display_brightness_level && display_brightness_task_run) {
            //display_brightness_task_run = false;
            uint8_t event = WAKEUP_EVENT_STOP;
            xQueueSend(display_queue, &event, 0);
        }
        //oledSetProfile(0);
        oled_screen_state(false);
    }
    display_power_state = on;
    return 0;
}

void display_timer_handler(void* context) {
    //TRACE_INFO("battery_timer_handler\r\n");

    portBASE_TYPE xHigherPriorityTaskWoken;
    uint8_t event = WAKEUP_EVENT_TIMED_EVENT;

    xQueueSendFromISR(display_queue, &event, &xHigherPriorityTaskWoken);

    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void display_brightness_task(void *p) {
    AnalogIn adc;
    bool running = true;
    bool timer_running = false;

    TRACE_INFO("display_brightness_task started\r\n");

    Timer timer;
    Timer_init(&timer, 0);
    Timer_setHandler(&timer, display_timer_handler, NULL);

    //display_brightness_task_run = false;
    //AnalogIn_init(&adc, 7);
    while (running) {
        uint8_t event;
        xQueueReceive(display_queue, &event, -1);
        switch (event) {
        case WAKEUP_EVENT_TIMED_EVENT:
            timer_running = false;
            break;
        case WAKEUP_EVENT_START:
            display_brightness_task_run = true;
            break;
        case WAKEUP_EVENT_STOP:
            display_brightness_task_run = false;
            break;
        case WAKEUP_EVENT_CLOSE:
            running = false; 
            display_brightness_task_run = false;
            break;
        }
        if (timer_running) {
            Timer_stop(&timer); 
        }
        if (display_brightness_task_run) {
            AnalogIn_init(&adc, 7);
            int value = AnalogIn_value(&adc);
            //int value = AnalogIn_valueWait(&adc);
            AnalogIn_close(&adc);

            //AnalogIn_test();
            //int value = 255;

            TRACE_INFO("display_brightness_task: value %d\r\n", value);
            int level = 255 - value;
            if (level >= 135) {
                level = 135 - 1;
            }
            level /= (135 / 3);

            TRACE_INFO("display_brightness_task: value %d level %d\r\n", value, level);

            scrLock();
            oledSetProfile(1 + level);
            scrUnLock();
     
            Timer_start(&timer, 1000, false, false);
            timer_running = true;
        }
    }
    //AnalogIn_close(&adc);
    TRACE_INFO("display_brightness_task stopped\r\n");
    Timer_close(&timer);
    vQueueDelete(display_queue);
    vTaskDelete(NULL);
}

int display_brightness(uint8_t level) {

    TRACE_INFO("display_brightness(%d)\r\n", level);

    if (level) {
        if (!display_brightness_level && display_brightness_task_run) {
            uint8_t event = WAKEUP_EVENT_STOP;
            xQueueSend(display_queue, &event, 0);
        }

        if (display_brightness_level != level || !display_power_state) {
            if (oledSetProfile(level)) {
                return 1;
            }
        }
    } else {
// TODO: auto brightness level
        if (display_brightness_level || !display_power_state) {
            uint8_t event = WAKEUP_EVENT_START;
            xQueueSend(display_queue, &event, 0);
        }
    }
    display_brightness_level = level;
    return 0;
}

int display_init() {
    display_queue = xQueueCreate(1, sizeof(uint8_t)); 
    if (display_queue == NULL) {
        panic("display_init");
        return -1;
    }

    //xTaskCreate(display_brightness_task, "display_brightness", TASK_STACK_SIZE(TASK_DISPLAY_BRIGHTNESS_STACK), NULL, TASK_DISPLAY_BRIGHTNESS_PRI, &display_brightness_task_handle);
    if (xTaskCreate(display_brightness_task, (signed char*)"display", TASK_STACK_SIZE(TASK_DISPLAY_BRIGHTNESS_STACK), NULL, TASK_DISPLAY_BRIGHTNESS_PRI, NULL) != 1) {
        return -1;
    }
    return 0;
}

int display_close() {
    uint8_t event = WAKEUP_EVENT_CLOSE;
    xQueueSend(display_queue, &event, 0);
    return 0;
}
