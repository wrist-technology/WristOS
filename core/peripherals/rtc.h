#ifndef RTC_H_
#define RTC_H_

// PA30 IRQ1

#define I2CRTC_PHY_ADDR (0x68)

#define I2CRTC_REGSEC10100  0x00
// b7 - b4 : 0.1sec
// b3 - b0 : 0.01sec

#define I2CRTC_REGSEC       0x01
// b7 : ST
// b6 - b4: 10sec
// b3 - b0: sec

#define I2CRTC_REGMIN       0x02
// b7: OFIE
// b6 - b4: 10min
// b3 - b0: min

#define I2CRTC_REGHR        0x03
// b7 - b6: 0
// b5 - b4: 10hour
// b3 - b0: hour (0-23)

#define I2CRTC_REGDAY       0x04
// RS3 RS2 RS1 RS0 0 dw2 dw1 dw0 (1-7)

#define I2CRTC_REGDATE      0x05
// 0 0 10date1 10date0 daymonth3-0 (1-31)

#define I2CRTC_REGCENMON     0x06
// CB1 CB0 0 10M month3-0 (0-3/1-12)

#define I2CRTC_REGYEAR     0x07
// 10y3-0 year3-0 (0-99)

#define I2CRTC_REGCALIB     0x08
// OUT 0 S calib4-0

#define I2CRTC_REGWD     0x09
// RB2 BMB4-0 RB1-0

#define I2CRTC_REGALM     0x0a
// AFE SQWE 0 AL10M alarmmonth3-0 (1-12)

#define I2CRTC_REGALD     0x0b
// RPT4 RPT5 AL10D1-0 alarmdate3-0 (1-31)

#define I2CRTC_REGALH     0x0c
// RPT3 0 AL10H1-0 alarmhour3-0 (0-23)

#define I2CRTC_REGALM     0x0d
// RPT2 AL10M2-0 alarmmin3-0 (0-59)

#define I2CRTC_REGALS     0x0e
// RPT1 AL10S2-0 alarmsec3-0 (0-59)

#define I2CRTC_REGGLAGS     0x0f
// WDF AF 0 0 0 OF 0 0

#define I2CRTC_STOPBIT 0x80

/*
1. Keys:
0 = must be set to '0'
AF = alarm flag (read only)
AFE = alarm flag enable flag
BMB0 - BMB4 = watchdog multiplier bits
CB0-CB1 = century bits
OF = oscillator fail bit
OFIE = oscillator fail interrupt enable bit
OUT = output level
RB0 - RB2 = watchdog resolution bits
RPT1-RPT5 = alarm repeat mode bits
RS0-RS3 = SQW frequency bits
S = sign bit
SQWE = square wave enable bit
ST = stop bit
WDF = watchdog flag bit (read only)
*/

#endif //RTC_H_
