#include "rtc.h"
#include "i2c.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "hardware_conf.h"
#include "debug/trace.h"
#include <time.h>

#define BCD2BIN(b) ((((b) & 0xf0) >> 4) * 10 + ((b) & 0x0f))
#define BIN2BCD(b) ((((b) / 10) << 4) | ((b) % 10))

static xSemaphoreHandle rtc_mutex;

int rtc_init() {
    rtc_mutex = xSemaphoreCreateMutex();
    if(rtc_mutex == NULL)
        panic("rtc_init");
    return 0;
}

int rtc_open() {
    i2c_open();
    return 0;
}

int rtc_close() {
    i2c_close();
    return 0;
}

int rtc_write(struct tm *new_time) {
    TRACE_RTC("rtc_write()\r\n");

    TRACE_RTC("rtc: %d.%02d.%02d %02d:%02d:%02d\r\n", new_time->tm_year + 1900, new_time->tm_mon + 1, new_time->tm_mday, new_time->tm_hour, new_time->tm_min, new_time->tm_sec);
    // rtc_wake();
    //portENTER_CRITICAL ();
    xSemaphoreTake(rtc_mutex, -1);

// TODO: sequential write to RTC
    i2cMasterWrite(I2CRTC_PHY_ADDR, 1, I2CRTC_REGSEC, I2CRTC_STOPBIT);

    i2cMasterWrite(I2CRTC_PHY_ADDR, 1, I2CRTC_REGDATE, BIN2BCD(new_time->tm_mday));

    uint16_t year = new_time->tm_year + 1900;
    uint8_t century = (year - 2000) / 100;
    uint8_t year_in_century = year % 100;
    i2cMasterWrite(I2CRTC_PHY_ADDR, 1, I2CRTC_REGCENMON, (century << 6) | BIN2BCD(new_time->tm_mon + 1));
    i2cMasterWrite(I2CRTC_PHY_ADDR, 1, I2CRTC_REGYEAR, BIN2BCD(year_in_century));

    i2cMasterWrite(I2CRTC_PHY_ADDR, 1, I2CRTC_REGHR, BIN2BCD(new_time->tm_hour));
    i2cMasterWrite(I2CRTC_PHY_ADDR, 1, I2CRTC_REGMIN, BIN2BCD(new_time->tm_min));
    i2cMasterWrite(I2CRTC_PHY_ADDR, 1, I2CRTC_REGSEC, BIN2BCD(new_time->tm_sec));

    //portEXIT_CRITICAL ();
    xSemaphoreGive(rtc_mutex);
    // rtc_sleep();
    return 0;
}

int rtc_read(struct tm *curr_time, unsigned int *milliseconds) {
    TRACE_RTC("rtc_read()\r\n");

#if 0
curr_time->tm_sec = 0;
curr_time->tm_min = 0;
curr_time->tm_hour = 18;
curr_time->tm_mday = 0;
curr_time->tm_mon = 1;
curr_time->tm_year = 110;
curr_time->tm_wday = 0;
curr_time->tm_yday = 0;
curr_time->tm_isdst = 0;
if (milliseconds) {
    *milliseconds = 0;
}
    
return 0;
#endif

    // rtc_wake();
    //portENTER_CRITICAL ();
    xSemaphoreTake(rtc_mutex, -1);

// TODO: sequential read from RTC
    if (milliseconds) {
        uint8_t b;
        i2cMasterRead(I2CRTC_PHY_ADDR, 1, I2CRTC_REGSEC10100, &b);
        *milliseconds = BCD2BIN(b) * 10;
    }

    uint8_t b;
    i2cMasterRead(I2CRTC_PHY_ADDR, 1, I2CRTC_REGSEC, &b);
    curr_time->tm_sec = BCD2BIN(b & 0x7f);
    i2cMasterRead(I2CRTC_PHY_ADDR, 1, I2CRTC_REGMIN, &b);
    curr_time->tm_min = BCD2BIN(b & 0x7f);
    i2cMasterRead(I2CRTC_PHY_ADDR, 1, I2CRTC_REGHR, &b);
    curr_time->tm_hour = BCD2BIN(b & 0x3f);
    i2cMasterRead(I2CRTC_PHY_ADDR, 1, I2CRTC_REGDATE, &b);
    curr_time->tm_mday = BCD2BIN(b & 0x3f);
    i2cMasterRead(I2CRTC_PHY_ADDR, 1, I2CRTC_REGCENMON, &b);
    uint8_t century = (b & 0xc0) >> 6;
    /*
century:
0 0 2000 (leap year; (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0))
0 1 2100
1 0 2200
1 1 2300
*/

    curr_time->tm_mon = BCD2BIN(b & 0x1f) - 1;
    i2cMasterRead(I2CRTC_PHY_ADDR, 1, I2CRTC_REGYEAR, &b);
    curr_time->tm_year = 2000 + century * 100 + BCD2BIN(b) - 1900;

    //portEXIT_CRITICAL ();
    xSemaphoreGive(rtc_mutex);

    // rtc_sleep();

    curr_time->tm_wday = 0;
    curr_time->tm_yday = 0;
    curr_time->tm_isdst = 0;

    TRACE_RTC("rtc: %d.%02d.%02d %02d:%02d:%02d\r\n", curr_time->tm_year + 1900, curr_time->tm_mon + 1, curr_time->tm_mday, curr_time->tm_hour, curr_time->tm_min, curr_time->tm_sec);
    return 0;
}

time_t rtc_get_epoch_seconds (unsigned int *milliseconds) {
    struct tm tm;
    TRACE_RTC("rtc_get_epoch_seconds()\r\n");

    rtc_read (&tm, milliseconds);
    return mktime (&tm);
}

int rtc_set_epoch_seconds (time_t now) {
    struct tm tm;

    TRACE_RTC("rtc_set_epoch_seconds(%d)\r\n", now);
    if (localtime_r (&now, &tm) == NULL)
        return -1;

    rtc_write (&tm);
    return 0;
}

int rtc_set_alarm (struct tm *tm)
{
    if (tm && (mktime (tm) < time (NULL)))
        return -1;

    return 0;
}

struct tm *rtc_get_alarm (struct tm *tm) {
}

time_t rtc_get_alarm_epoch_seconds (void) {
    struct tm tm;

    return mktime (rtc_get_alarm (&tm));
}

int rtc_periodic_alarm (int mode) {
    return 0;
}
