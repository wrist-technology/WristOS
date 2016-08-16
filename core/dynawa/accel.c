#include "io.h"
#include "accel.h"
#include "debug/trace.h"
#include "event.h"
#include "task.h"
#include "queue.h"
#include "task_param.h"

#define WAKEUP_EVENT_CLOSE          1
#define WAKEUP_EVENT_ACCEL          20

#define PIO_ACCEL  IO_PB20

static xTaskHandle accel_task_handle;
xQueueHandle accel_queue;

static Io accel_io;
static uint32_t last_gesture = 0;

static bool accel_wakeup_pin_high = false;

void accel_io_isr_handler(void* context) {
    //accel_wakeup_pin_high = !accel_wakeup_pin_high;
    accel_wakeup_pin_high = Io_value(&accel_io);

    if (!accel_wakeup_pin_high) {
        return;
    }

    portBASE_TYPE xHigherPriorityTaskWoken;
    uint8_t ev = WAKEUP_EVENT_ACCEL;
    xQueueSendFromISR(accel_queue, &ev, &xHigherPriorityTaskWoken);

    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void accel_task( void* p ) {
    TRACE_INFO("accel task %x\r\n", xTaskGetCurrentTaskHandle());
    
    Io_init(&accel_io, PIO_ACCEL, IO_GPIO, INPUT);
    Io_addInterruptHandler(&accel_io, accel_io_isr_handler, NULL);
#if 0
    accel_stop();

    Task_sleep(10);

#endif
    accel_start();

    Task_sleep(10);

    while (true) {

        uint8_t accel_ev;
        xQueueReceive(accel_queue, &accel_ev, -1);
    
        if (accel_ev == WAKEUP_EVENT_CLOSE) {
            break;
        }

        spi_lock();
        uint8_t src = accel_reg_read8(ACCEL_REG_DD_SRC);

        int16_t x = 0, y = 0, z = 0;
        accel_read(&x, &y, &z, false);

        accel_reg_read8(ACCEL_REG_DD_ACK);
        spi_unlock();

        TRACE_ACCEL("accel %x %d %d %d\r\n", src, x, y, z);

        uint32_t gesture = src & 0x04 ? 1 : 2;

        if (!last_gesture || last_gesture != gesture) {
            last_gesture = gesture;

            event ev;
            ev.type = EVENT_ACCEL;
            ev.data.accel.gesture = gesture;
            ev.data.accel.x = x;
            ev.data.accel.y = y;
            ev.data.accel.z = z;
            event_post(&ev);
        }
    }
    accel_stop();
    Io_removeInterruptHandler(&accel_io);
    vQueueDelete(accel_queue);
    vTaskDelete(NULL);
}

int accel_init () {
    accel_queue = xQueueCreate(1, sizeof(uint8_t));
    if (accel_queue == NULL) {
        panic("accel_init");
        return -1;
    }

    if (xTaskCreate( accel_task, "accel", TASK_STACK_SIZE(TASK_ACCEL_STACK), NULL, TASK_ACCEL_PRI, &accel_task_handle ) != 1 ) {
        return -1;
    }

    return 0;
}

int accel_close() {
    uint8_t ev = WAKEUP_EVENT_CLOSE;
    xQueueSend(accel_queue, &ev, 0);
}

