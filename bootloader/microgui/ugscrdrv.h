/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */


//uGUI screen driver

#ifndef _UGSCRDRV_H_
#define _UGSCRDRV_H_

#define UGD_FONT8x8 0
#define UGD_FONT6x8 1
#define UGD_FONT10x14 2

#include "scroutput.h"

#ifndef UGD_STDOUT 
#include <inttypes.h> 
#endif

typedef uint8_t tugd_Coord; //small displays
typedef uint32_t tugd_Color; //truecolor
typedef uint8_t tugd_Font;

typedef scr_bitmapbuf_t tugd_Bitmap;

extern void ugd_rect(tugd_Coord x1, tugd_Coord y1, tugd_Coord x2, tugd_Coord y2);
extern void ugd_bitmap(tugd_Coord x1, tugd_Coord y1, tugd_Coord w, tugd_Coord h, tugd_Bitmap *bm);

extern void ugd_line(tugd_Coord x1, tugd_Coord y1, tugd_Coord x2, tugd_Coord y2);

extern void ugd_putchar(char c);

extern void ugd_print(tugd_Coord x, tugd_Coord y, char *c);

extern uint32_t ugd_charlen(void);
extern uint32_t ugd_charh(void);

extern tugd_Font ugdfont;
extern tugd_Color ugdfgColor;
extern tugd_Color ugdbgColor;

#endif

