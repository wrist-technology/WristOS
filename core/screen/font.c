/* ----------------------------------------------------------------------------
 *     ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

//------------------------------------------------------------------------------
//     Headers
//------------------------------------------------------------------------------

#include "firmware_conf.h"
#include <debug/assert.h>
#include "font10x14.h"
#include "font8.h"
#include "font8x8.h"
#include "screen.h"
#include "font.h"


//------------------------------------------------------------------------------
//     Local variables
//------------------------------------------------------------------------------

/// Global variable describing the font being instancied.
//const Font gFont = {10, 14};

scr_color_t fontColor=0xffffff;
scr_color_t fontBgColor=0x000000;
scr_coord_t fontCurrPosX=0;
scr_coord_t fontCarridgeReturnPosX=0;
scr_coord_t fontCurrPosY=0;
scr_bitmapbuf_t fontBuf[(10*14)]; 

//------------------------------------------------------------------------------
//     Global functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Draws an ASCII character on the given LCD buffer.
/// \param pBuffer  Buffer to write on.
/// \param x  X-coordinate of character upper-left corner.
/// \param y  Y-coordinate of character upper-left corner.
/// \param c  Character to output.
/// \param color  Character color.
//------------------------------------------------------------------------------



void fontSetCharPos(scr_coord_t x, scr_coord_t y)
{
  fontCurrPosX=x;
  fontCurrPosY=y;
}

void fontSetCarridgeReturnPosX(scr_coord_t x)
{
  fontCarridgeReturnPosX=x;
}

void fontSetColor(scr_color_t fg, scr_color_t bg) {
    fontColor = fg;
    fontBgColor = bg;
}

void fontPutChar(char c)
{
    unsigned int row=0;
    unsigned int col;
    
    if (c=='\n') { fontCurrPosY+=8; return; }
    if (c=='\r') { fontCurrPosX=fontCarridgeReturnPosX; return; }
    for (row = 0; row < 8; row++)        
    {
        for (col = 0; col < 8; col++)
        {
          if ((pCharset8x8[c*8+row] >> (7-col)) & 0x1) 
          {
             fontBuf[(row*8)+col] = fontColor;                
            } else {
               fontBuf[(row*8)+col] = fontBgColor; 
            }
        }
    }
    
    //for(col=0; col<(4*13); col++) { fontBuf[col*2]=fontColor; }
    //print bitmap to oled
    //scrWriteBitmap(fontCurrPosX,fontCurrPosY,fontCurrPosX+9,fontCurrPosY+13,fontBuf);
    scrWriteBitmap(fontCurrPosX,fontCurrPosY,fontCurrPosX+7,fontCurrPosY+7,fontBuf);
    //fontCurrPosX+=12;
    fontCurrPosX+=8;
}

#if 0
void fontPutChar(char c)
{
    unsigned int row, col,l;

    if ((c >= 0x20) && (c <= 0x7F))
    {
    /*
    for (col = 0; col < 10; col++) {

        for (row = 0; row < 8; row++) {

            if ((pCharset10x14[((c - 0x20) * 20) + col * 2] >> (7 - row)) & 0x1) 
            {
               fontBuf[(row*10)+col] = fontColor;                
            } else {
               fontBuf[(row*10)+col] = fontBgColor; 
            }
        }
        for (row = 0; row < 6; row++) 
        {

            if ((pCharset10x14[((c - 0x20) * 20) + col * 2 + 1] >> (7 - row)) & 0x1) 
            {
               fontBuf[(row+8)*10+col] = fontColor;

                //LCDD_DrawPixel(pBuffer, col, row+8, fontColor);
            } else {
               fontBuf[(row+8)*10+col] = fontBgColor; 
            }
        }
    }
    */
    l = (font8offsets[c - 0x20+1]-font8offsets[c - 0x20]);
    
    
    for (col = 0; col < l; col++)
    {
      for (row = 0; row < 8; row++)        
        {
          if ((pCharset8[font8offsets[c - 0x20] + col] >> (row)) & 0x1) 
          {
             fontBuf[(row*l)+col] = fontColor;                
            } else {
               fontBuf[(row*l)+col] = fontBgColor; 
            }
        }
    }
    
    /////////
    } else {
      for(col=0; col<70; col++) { fontBuf[col*2]=fontColor; }
      for(col=0; col<70; col++) { fontBuf[col*2+1]=fontBgColor; }    
    }
    //for(col=0; col<(4*13); col++) { fontBuf[col*2]=fontColor; }
    //print bitmap to oled
    //scrWriteBitmap(fontCurrPosX,fontCurrPosY,fontCurrPosX+9,fontCurrPosY+13,fontBuf);
    scrWriteBitmap(fontCurrPosX,fontCurrPosY,fontCurrPosX+l-1,fontCurrPosY+7,fontBuf);
    //fontCurrPosX+=12;
    fontCurrPosX+=(l+1);
}
#endif
