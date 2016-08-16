/* ========================================================================== */
/*                                                                            */
/*   oled.c                                                                   */
/*   (c) 2009 Wrist Technology Ltd                                           */
/*                                                                            */
/*   OLED display driver                                                      */
/*                                                                            */
/* ========================================================================== */

#ifndef OLED_H
#define OLED_H

//#define rgb2w(r,g,b) ((b&0x3F)|((g<<6)&0xFC0)|((r<<12)&0x3F000))

#define rgb2w(r,g,b) (((b&0x3F)<<16)|((g&0x7)<<22)|((g>>3)&0x7)|((r&0x3f)<<3))
//#define rgb2w(r,g,b) (((b&0x3F)<<16)|((g&0x7)<<22)|((g>>3)&0x7)|((r&0x3f)<<3))


/**  
 *  Display general info
 *
*/
#define OLED_RESOLUTION_Y 128
#define OLED_RESOLUTION_X 160

#define OLED_CMD_BASE 0x20000000

#define OLED_PARAM_BASE 0x20080000

/**
 * Internal OLED controller (SEPS525) settings:
 *
 *  
*/
#define STATUS_RD      0x01
#define OSC_CTL        0x02
#define CLOCK_DIV      0x03
#define REDUCE_CURRENT 0x04
#define SOFT_RST       0x05
#define DISP_ON_OFF    0x06
#define PRECHARGE_TIME_R 0x08
#define PRECHARGE_TIME_G 0x09
#define PRECHARGE_TIME_B 0x0A

#define PRECHARGE_CURRENT_R 0x0B
#define PRECHARGE_CURRENT_G 0x0C
#define PRECHARGE_CURRENT_B 0x0D


#define DRIVING_CURRENT_R 0x10
#define DRIVING_CURRENT_G 0x11
#define DRIVING_CURRENT_B 0x12

#define DISPLAY_MODE_SET 0x13
#define RGB_IF           0x14

#define RGB_POL          0x15

#define MEMORY_WRITE_MODE 0x16

#define MX1_ADDR          0x17
#define MX2_ADDR          0x18
#define MY1_ADDR          0x19
#define MY2_ADDR          0x1A
#define MEMORY_ACCESSP_X 0x20
#define MEMORY_ACCESSP_Y 0x21
#define OLED_DUTY         0x28
#define OLED_DSL          0x29
#define OLED_IREF         0x80

#define OLED_DDRAM        0x22

typedef  unsigned long long int oled_access_fast;
typedef  unsigned short int oled_access_cmd;
typedef  unsigned int oled_access;

typedef struct {
    uint8_t precharge_time_r, precharge_time_g, precharge_time_b;
    uint8_t precharge_current_r, precharge_current_g, precharge_current_b;
    uint8_t driving_current_r, driving_current_g, driving_current_b;
} oled_profile;

int oledInitHw(void);
void oledWriteCommand(uint16_t cmd, uint16_t param);
void oledWrite(uint16_t param);



/*
    11010101
    00010100
    10011001
    01101010
    00000000
    00001011
    01111101
    11000101
    11101101
    
    
7   x  
6   x
5   
4   x  x
3         x
2   x  x  
1   
0   x     x

*/

#endif  // OLED_H
