/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */


#include "scroutput.h"

//uGUI screen driver

#ifdef UGD_STDOUT
#include <stdio.h>
#include <stdint.h>
//#include "ints.h"
#else
//#include <inttypes.h>
#include "hardware_conf.h"
#include "firmware_conf.h"
#include <screen/screen.h>
#include <debug/trace.h>
#include <screen/font.h>
#endif

#include "ugscrdrv.h"

tugd_Coord xchar,ychar; //current char position
tugd_Font ugdfont;
tugd_Color ugdfgColor;
tugd_Color ugdbgColor; 

void ugd_rect(tugd_Coord x1, tugd_Coord y1, tugd_Coord x2, tugd_Coord y2)
{
  #ifdef UGD_STDOUT
  printf("[RECT %d,%d:%d,%d|B.%x|F.%x]",x1,y1,x2,y2,ugdbgColor,ugdfgColor);
  #else
  //TRACE_ALL("[]");
  if (x2>x1)
  { 
    if (y2>y1) 
      scrWriteRect(x1,y1,x2,y2,ugdbgColor);
    else 
      scrWriteRect(x1,y2,x2,y1,ugdbgColor); 
  } else {
    if (y2>y1) 
      scrWriteRect(x2,y1,x1,y2,ugdbgColor);
    else 
      scrWriteRect(x2,y2,x1,y1,ugdbgColor);  
  }  
  #endif
}

void ugd_bitmap(tugd_Coord x1, tugd_Coord y1, tugd_Coord w, tugd_Coord h, tugd_Bitmap *bm)
{
  #ifdef UGD_STDOUT
  printf("[BITMAP %d,%d:%d,%d]",x1,y1,w,h);
  #else
  //TRACE_ALL("[]");
  scrWriteBitmap(x1,y1,x1+w-1,y1+h-1,bm);
  #endif
}

void ugd_line(tugd_Coord x1, tugd_Coord y1, tugd_Coord x2, tugd_Coord y2)
{
  #ifdef UGD_STDOUT
  printf("[LINE %d,%d:%d,%d]",x1,y1,x2,y2);
  #else
  scrWriteRect(x1,y1,x1,y2,ugdfgColor);
  #endif
}

void ugd_putchar(char c)
{
  #ifdef UGD_STDOUT
  printf("%c",c);
  #else
  switch (ugdfont)
  {
    case UGD_FONT8x8: fontPutChar(c); break;//default
    case UGD_FONT6x8: fontPutChar6x8(c); break;
    case UGD_FONT10x14: fontPutChar10x14(c); break;
    default: fontPutChar(c); break;
  }
  #endif
}

uint32_t ugd_charlen(void)
{
  #ifdef UGD_STDOUT
  
  #else
  switch (ugdfont)
  {
    case UGD_FONT8x8: return 8;
    case UGD_FONT6x8: return 6;
    case UGD_FONT10x14: return 10;
    default: return 8;
  }
  #endif
}

uint32_t ugd_charh(void)
{
  #ifdef UGD_STDOUT
  
  #else
  switch (ugdfont)
  {
    case UGD_FONT8x8: return 8;
    case UGD_FONT6x8: return 8;
    case UGD_FONT10x14: return 14;
    default: return 8;
  }
  #endif
}

void ugd_print(tugd_Coord x, tugd_Coord y, char *c)
{
  xchar = x;
  ychar = y; 
  #ifdef UGD_STDOUT
  printf("[PRN ");
  #else
  fontColor=ugdfgColor;
  fontBgColor=ugdbgColor;
  fontSetCharPos(x, y);    
  #endif
      
  while (*c)
  {
    ugd_putchar(*c);
    c++;
  }
  
  #ifdef UGD_STDOUT
  printf("|B.%x|F.%x]\n",ugdbgColor,ugdfgColor);    
  #endif
}

