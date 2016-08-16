/* ========================================================================== */
/*                                                                            */
/*   screen.h                                                               */
/*   (c) 2009 Wrist Technology Ltd                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

//'do not fill' colour:



#ifndef SCREEN_H

#define SCREEN_H

//#define SCR_COLORDEPTH 8

//#include "firmware_conf.h"
#include <inttypes.h> 

#define SCR_TRANSPARENT_COLOR 0x01000000
#define SCR_COLOR_RED    0x000000ff
#define SCR_COLOR_BLUE   0x00ff0000
#define SCR_COLOR_GREEN  0x0000ff00
#define SCR_COLOR_YELLOW 0x00ffff00
#define SCR_COLOR_WHITE  0x00ffffff


typedef int16_t scr_coord_t;
extern uint8_t scrrot, scrmirror;
 

#if (SCR_COLORDEPTH==32)
typedef uint32_t scr_buf_t;
typedef uint32_t scr_color_t;
typedef uint32_t scr_bitmapbuf_t; 
#elif (SCR_COLORDEPTH==8)
typedef uint8_t scr_buf_t;
typedef uint8_t scr_color_t;
typedef uint8_t scr_bitmapbuf_t; 
#endif

#ifndef SCR_COLORDEPTH
#error Define SCR_COLORDEPTH!
#endif

//extern scr_buf_t scrbuf;

/**
 *  Screen driver - standard interface
 *
 *
 *   
*/

extern int scrInit(void);

//screen in physical screen:
//int scrScr(scr_coord_t left_x, scr_coord_t top_y, scr_coord_t right_x, scr_coord_t bot_y);

//int scrSleep(uint8_t mode); //blanking, deepsleep with content etc

//int scrWakeup(void);

//int scrPowerOff(void); //power off, requires init

//int scrRotate(uint16_t rot);

//int scrMirror(uint8_t vert, uint8_t horiz);

//roll screen up+ down-, right+, left-
//int scrRoll(uint8_t axis, scr_coord_t); 

//void scrBuffering(scr_buf_t scrbuf);  //draw into memory; copy to display on scrRefresh() scrbuf=NULL - nobuffering (on init)

//int scrRefresh(void); //if memory buffering used, copy mem to screen (typ. on VSYNC pulse)
 
//int scrBrightness(unsigned int brightness);

//int scrContrast(unsigned int contrast);

//int scrTemperature(unsigned int temperature);

//void scrWritePixel(scr_coord_t x, scr_coord_t y, scr_color_t color);

//scr_color_t scrReadPixel(scr_coord_t x, scr_coord_t y);

extern void scrWriteRect(scr_coord_t left_x, scr_coord_t top_y, scr_coord_t right_x, scr_coord_t bot_y, scr_color_t color);

extern void scrWriteBitmap(scr_coord_t left_x, scr_coord_t top_y, scr_coord_t right_x, scr_coord_t bot_y, scr_bitmapbuf_t *buf);

//void scrReadBitmap(scr_coord_t left_x, scr_coord_t top_y, scr_coord_t right_x, scr_coord_t bot_y, scr_bitmapbuf_t buf);

//void scrWriteTransparentBitmap(scr_coord_t left_x, scr_coord_t top_y, scr_coord_t right_x, scr_coord_t bot_y, scr_bitmapbuf_t buf); 
#endif