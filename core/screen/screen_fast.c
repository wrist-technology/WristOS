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

#define SCREEN_ROTATE   1

extern scr_buf_t * scrbuf; 
#if 0
scr_coord_t scrscrX1;
scr_coord_t scrscrX2;
scr_coord_t scrscrY1;
scr_coord_t scrscrY2;
uint16_t scrrot, scrmirror;

//scr buffer, allocate mem!
extern scr_buf_t * scrbuf; 

//note: requires the SMC pins (AD0.., D0..D15, R/W) already configured!
int scrInit(void)
{
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
#endif

void scrWriteRect(scr_coord_t left_x, scr_coord_t top_y, scr_coord_t right_x, scr_coord_t bot_y, scr_color_t color)
{
    scr_coord_t x1,x2,y1,y2;
    volatile oled_access_fast *pFastOLED;
    oled_access_fast col;
    volatile oled_access_cmd *pOLED;
    uint32_t writes,i;


    if (scrbuf==NULL)
    {
        //direct to screen
        if (left_x>=0) x1 = left_x; else x1=0; if (left_x<OLED_RESOLUTION_X) x1 = left_x; else x1=OLED_RESOLUTION_X-1;
        if (right_x>=0) x2 = right_x; else x2=0; if (right_x<OLED_RESOLUTION_X) x2 = right_x; else x2=OLED_RESOLUTION_X-1;
        if (top_y>=0) y1 = top_y; else y1=0; if (top_y<OLED_RESOLUTION_Y) y1 = top_y; else y1=OLED_RESOLUTION_Y-1;
        if (bot_y>=0) y2 = bot_y; else y2=0; if (bot_y<OLED_RESOLUTION_Y) y2 = bot_y; else y2=OLED_RESOLUTION_Y-1;

        //clear screen (blank, black)
        oledWriteCommand(MX1_ADDR, x1);
        oledWriteCommand(MY1_ADDR, y1);
        oledWriteCommand(MX2_ADDR, x2);
        oledWriteCommand(MY2_ADDR, y2);
        oledWriteCommand(MEMORY_ACCESSP_X, x1);
        oledWriteCommand(MEMORY_ACCESSP_Y, y1);  
        pOLED=OLED_CMD_BASE;  
        *pOLED = (OLED_DDRAM<<1); //bit align          
        pFastOLED = OLED_PARAM_BASE;
        writes = (((x2-x1+1)*(y2-y1+1))/8)+1;   
        col = rgb2w((color&0xff),(color>>8)&0xff,(color>>16)&0xff);
        col = col | (col<<32);
        //this is ok - if rectangle is < 8pix overwrites picture many times          
        for (i=0;i<(writes);(i++))
        {    
            *pFastOLED = col;
            *pFastOLED = col;
            *pFastOLED = col;
            *pFastOLED = col;            
        }

    } else {
        //write to screen memory buffer 

    }
}


void scrWriteBitmap(scr_coord_t left_x, scr_coord_t top_y, scr_coord_t right_x, scr_coord_t bot_y, scr_bitmapbuf_t *buf)
{
    scr_coord_t x1,x2,y1,y2;
    //volatile oled_access_fast *pFastOLED;  
    volatile oled_access_cmd *pCMDOLED;
    volatile oled_access *pOLED;
    uint32_t writes,i;

    if (scrbuf==NULL)
    {
        if (left_x>=0) x1 = left_x; else x1=0; if (left_x<OLED_RESOLUTION_X) x1 = left_x; else x1=OLED_RESOLUTION_X-1;
        if (right_x>=0) x2 = right_x; else x2=0; if (right_x<OLED_RESOLUTION_X) x2 = right_x; else x2=OLED_RESOLUTION_X-1;
        if (top_y>=0) y1 = top_y; else y1=0; if (top_y<OLED_RESOLUTION_Y) y1 = top_y; else y1=OLED_RESOLUTION_Y-1;
        if (bot_y>=0) y2 = bot_y; else y2=0; if (bot_y<OLED_RESOLUTION_Y) y2 = bot_y; else y2=OLED_RESOLUTION_Y-1;

        //direct to screen
        //clear screen (blank, black)
        oledWriteCommand(MX1_ADDR, x1);
        oledWriteCommand(MY1_ADDR, y1);
        oledWriteCommand(MX2_ADDR, x2);
        oledWriteCommand(MY2_ADDR, y2);
        oledWriteCommand(MEMORY_ACCESSP_X, x1);
        oledWriteCommand(MEMORY_ACCESSP_Y, y1);  
        pCMDOLED=OLED_CMD_BASE;  
        *pCMDOLED = (OLED_DDRAM<<1); //bit align

        pOLED = OLED_PARAM_BASE;
        writes = (((x2-x1+1)*(y2-y1+1)));   

        for (i=0;i<(writes);(i++))
        {          
            *pOLED = rgb2w((buf[i]&0xff),(buf[i]>>8)&0xff,(buf[i]>>16)&0xff);;                  
            //*pOLED = rgb2w(0xff,0xff,0xff);
        }

    } else {
        //write to screen memory buffer 

    }
}

void (*_rprintf)() = rprintf;
#define _TRACE_INFO(...)         (*_rprintf)(DBG,__VA_ARGS__)


void scrWriteBitmapRGBA(scr_coord_t left_x, scr_coord_t top_y, scr_coord_t right_x, scr_coord_t bot_y, scr_bitmapbuf_t *buf)
{
    scr_coord_t x1,x2,y1,y2;
    //volatile oled_access_fast *pFastOLED;  
    volatile oled_access_cmd *pCMDOLED;
    volatile oled_access *pOLED;
    uint32_t writes;
    int32_t i;

    //_TRACE_INFO("scrWriteBitmapRGBA %d %d %d %d %x\r\n", left_x, top_y, right_x, bot_y, buf);

    if (scrbuf==NULL)
    {
        if (left_x>=0) x1 = left_x; else x1=0; if (left_x<OLED_RESOLUTION_X) x1 = left_x; else x1=OLED_RESOLUTION_X-1;
        if (right_x>=0) x2 = right_x; else x2=0; if (right_x<OLED_RESOLUTION_X) x2 = right_x; else x2=OLED_RESOLUTION_X-1;
        if (top_y>=0) y1 = top_y; else y1=0; if (top_y<OLED_RESOLUTION_Y) y1 = top_y; else y1=OLED_RESOLUTION_Y-1;
        if (bot_y>=0) y2 = bot_y; else y2=0; if (bot_y<OLED_RESOLUTION_Y) y2 = bot_y; else y2=OLED_RESOLUTION_Y-1;

        //direct to screen
        //clear screen (blank, black)
        writes = (((x2-x1+1)*(y2-y1+1)));   

        if (SCREEN_ROTATE) {
            oledWriteCommand(MX1_ADDR, x1);
            oledWriteCommand(MY1_ADDR, y1);
            oledWriteCommand(MX2_ADDR, x2);
            oledWriteCommand(MY2_ADDR, y2);
            oledWriteCommand(MEMORY_ACCESSP_X, x1);
            oledWriteCommand(MEMORY_ACCESSP_Y, y1);  
        } else {
            oledWriteCommand(MX1_ADDR, OLED_RESOLUTION_X - x2 - 1);
            oledWriteCommand(MY1_ADDR, OLED_RESOLUTION_Y - y2 - 1);
            oledWriteCommand(MX2_ADDR, OLED_RESOLUTION_X - x1 - 1);
            oledWriteCommand(MY2_ADDR, OLED_RESOLUTION_Y - y1 - 1);
            oledWriteCommand(MEMORY_ACCESSP_X, OLED_RESOLUTION_X - x2 - 1);
            oledWriteCommand(MEMORY_ACCESSP_Y, OLED_RESOLUTION_Y - y2 - 1);  
        }
        pCMDOLED=OLED_CMD_BASE;  
        *pCMDOLED = (OLED_DDRAM<<1); //bit align

        pOLED = OLED_PARAM_BASE;

// Bitmap RGBA (6b channel)
// 10987654321098765432109876543210
// 32103210321032103210321032103210
// AAAAAAAABBBBBB--GGGGGG--RRRRRR--
// 76543210765432--765432--765432--

// OLED Controller Write
// 10987654321098765432109876543210
// 32103210321032103210321032103210
// -------GGGBBBBBB-------RRRRRRGGG
// -------210543210-------543210543

#define rgb21w(r,g,b) ((((b)&0x3F)<<16)|(((g)&0x7)<<22)|(((g)>>3)&0x7)|(((r)&0x3f)<<3))
#define rgb62w(r,g,b) ((((b)&0xfc)<<14)|(((g)&0x1c)<<20)|(((g)>>5)&0x7)|(((r)&0xfc)<<1))
#define RGBA2W(rgba)    ((((rgba) & 0xfc) << 1) | (((rgba) & 0xfc0000) >> 2) | (((rgba) & 0x1c00) << 12) | (((rgba) & 0xe000) >> 13))

/*
        _TRACE_INFO("%x\r\n", rgb2w(0xfc>>2, 0xfc>>2, 0xfc>>2));
        _TRACE_INFO("%x\r\n", rgb62w(0xfc, 0xfc, 0xfc));
        _TRACE_INFO("%x\r\n", rgba2w(0xfcfcfc));
*/
        if (SCREEN_ROTATE) {
            for (i=0;i<(writes);(i++)) {          
                // 24 - 31b - Alpha
                //*pOLED = rgb2w((buf[i]&0xff),(buf[i]>>8)&0xff,(buf[i]>>16)&0xff);                  
                //*pOLED = rgb21w((buf[i]&0xff),(buf[i]>>8)&0xff,(buf[i]>>16)&0xff);                  
                //register uint32_t rgba = (buf[i] & 0xfcfcfc) >> 2;
                //*pOLED = rgb2w((rgba&0xff),(rgba>>8)&0xff,(rgba>>16)&0xff);                  
                //*pOLED = rgb62w((buf[i]&0xff),(buf[i]>>8)&0xff,(buf[i]>>16)&0xff);                  
                // [RGB] 8b -> 6b
                //*pOLED = rgb2w((buf[i]&0xff)>>2,((buf[i]>>8)&0xff)>>2,((buf[i]>>16)&0xff)>>2);                  
                //*pOLED = buf[i];
                *pOLED = RGBA2W(buf[i]);
                //*pOLED = 0x11223344;
            }
        } else {
            for (i = writes - 1; i >= 0; i--) {
                *pOLED = RGBA2W(buf[i]);
            }
        }

    } else {
        //write to screen memory buffer 

    }
}

void scrWriteBitmapRGBA2(scr_coord_t scr_x, scr_coord_t scr_y, scr_coord_t bmp_x, scr_coord_t bmp_y, uint16_t width, uint16_t height, bitmap *bmp)
{
    scr_coord_t x1,x2,y1,y2;
    //volatile oled_access_fast *pFastOLED;  
    volatile oled_access_cmd *pCMDOLED;
    volatile oled_access *pOLED;

    //_TRACE_INFO("scrWriteBitmapRGBA2 [%d %d] [%d %d] [%d %d] %x\r\n", scr_x, scr_y, bmp_x, bmp_y, width, height, bmp);

    uint16_t bmp_width = bmp->header.width;
    uint16_t bmp_height = bmp->header.height;

    if (scr_x >= OLED_RESOLUTION_X || scr_y >= OLED_RESOLUTION_Y)
        return;

    if (scr_x < 0) {
        width += scr_x;
        scr_x = 0;
    }
    
    if (scr_y < 0) {
        height += scr_y;
        scr_y = 0;
    }

    if (scr_x + width > OLED_RESOLUTION_X) {
        width = OLED_RESOLUTION_X - scr_x;
    }
    if (scr_y + height > OLED_RESOLUTION_Y) {
        height = OLED_RESOLUTION_Y - scr_y;
    }
    //_TRACE_INFO("scrWriteBitmapRGBA2 [%d %d] [%d %d] [%d %d] %x\r\n", scr_x, scr_y, bmp_x, bmp_y, width, height, bmp);
    if (!bitmap_check_bounds(bmp, &bmp_x, &bmp_y, &width, &height)) {
        return;
    }
    //_TRACE_INFO("scrWriteBitmapRGBA2 [%d %d] [%d %d] [%d %d] %x\r\n", scr_x, scr_y, bmp_x, bmp_y, width, height, bmp);

    x1 = scr_x;
    y1 = scr_y;

    x2 = x1 + width - 1;
    y2 = y1 + height - 1;

    if (SCREEN_ROTATE) {
        oledWriteCommand(MX1_ADDR, x1);
        oledWriteCommand(MY1_ADDR, y1);
        oledWriteCommand(MX2_ADDR, x2);
        oledWriteCommand(MY2_ADDR, y2);
        oledWriteCommand(MEMORY_ACCESSP_X, x1);
        oledWriteCommand(MEMORY_ACCESSP_Y, y1);  
    } else {
        oledWriteCommand(MX1_ADDR, OLED_RESOLUTION_X - x2 - 1);
        oledWriteCommand(MY1_ADDR, OLED_RESOLUTION_Y - y2 - 1);
        oledWriteCommand(MX2_ADDR, OLED_RESOLUTION_X - x1 - 1);
        oledWriteCommand(MY2_ADDR, OLED_RESOLUTION_Y - y1 - 1);
        oledWriteCommand(MEMORY_ACCESSP_X, OLED_RESOLUTION_X - x2 - 1);
        oledWriteCommand(MEMORY_ACCESSP_Y, OLED_RESOLUTION_Y - y2 - 1);  
    }
    pCMDOLED=OLED_CMD_BASE;  
    *pCMDOLED = (OLED_DDRAM<<1); //bit align

    pOLED = OLED_PARAM_BASE;


    uint32_t *rgba_data = (uint32_t*)((uint8_t*)bmp + sizeof(bitmap_header));

    uint32_t bmp_addr_wrap = bmp_width - width;

    int x, y;
    if (SCREEN_ROTATE) {
        uint32_t bmp_addr = bmp_y * bmp_width + bmp_x;
        for (y = 0; y < height; y++) {          
            for (x = 0; x < width; x++) {          
                *pOLED = RGBA2W(rgba_data[bmp_addr]);
                bmp_addr++;
            }
            bmp_addr += bmp_addr_wrap;
        }
    } else {
        uint32_t bmp_addr = (bmp_y + height - 1) * bmp_width + bmp_x + width - 1;
        for (y = height - 1; y >= 0; y--) {          
            for (x = width - 1; x >= 0; x--) {          
                *pOLED = RGBA2W(rgba_data[bmp_addr]);
                bmp_addr--;
            }
            bmp_addr -= bmp_addr_wrap;
        }
    }
}
