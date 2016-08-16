/* ========================================================================== */
/*                                                                            */
/*   screen.c                                                               */
/*   (c) 2009 Wrist Technology Ltd                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */


/**
 *  Screen driver
 *
 *
 *   
 */

#include <stdio.h>
#include "hardware_conf.h"
#include "firmware_conf.h"
#include "utils/delay.h"
#include "peripherals/oled/oled.h"
#include "screen.h"
#include "debug/trace.h"
#include "bitmap.h"
#include "rtos.h"

scr_coord_t scrscrX1;
scr_coord_t scrscrX2;
scr_coord_t scrscrY1;
scr_coord_t scrscrY2;
uint16_t scrrot, scrmirror;

//scr buffer, allocate mem!
extern scr_buf_t * scrbuf;

static xSemaphoreHandle screen_mutex;

void scrLock() {
    xSemaphoreTake(screen_mutex, -1);
}

void scrUnLock() {
    xSemaphoreGive(screen_mutex);
}

//note: requires the SMC pins (AD0.., D0..D15, R/W) already configured!
int scrInit(void)
{
    screen_mutex = xSemaphoreCreateMutex();
    if (screen_mutex == NULL) {
        panic("scrInit");
    }

    //init hw, switch power on and clear display
    (void) oledInitHw();

    //configure screen window
    scrscrX1=0; scrscrX2=OLED_RESOLUTION_X-1;
    scrscrY1=0; scrscrY2=OLED_RESOLUTION_Y-1;

    //oledScreen(scrscrX1,scrscrY1,scrscrX2,scrscrY2);
    scrbuf = NULL;
    scrrot = 0;
    scrmirror = 0;

    return 0;
}

void scrClose() {
    vQueueDelete(screen_mutex);
}
