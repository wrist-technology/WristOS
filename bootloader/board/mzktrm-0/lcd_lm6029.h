/* ========================================================================== */
/*                                                                            */
/*   lcd_lm6029.h                                                            */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */
#define SCR_RESOLUTION_X 128
#define SCR_RESOLUTION_Y 64

#define LCDI_RESET 0xE2

#define LCDI_DISPON 0xAF
#define LCDI_DISPOFF 0xAE

#define LCDI_STARTLINE 0x40
#define LCDI_STARTLINE_M 0x3F

#define LCDI_SETPAGE 0xB0
#define LCDI_SETPAGE_M 0x0F

#define LCDI_SETCOLH 0x10
#define LCDI_SETCOLH_M 0x0F
#define LCDI_SETCOLL 0x00
#define LCDI_SETCOLL_M 0x0F

#define LCDI_ADCSEL_NORM 0xA0
#define LCDI_ADCSEL_REV  0xA1

#define LCDI_DISP_NORM 0xA6
#define LCDI_DISP_REV  0xA7

#define LCDI_ENTIRE_ON 0xA5
#define LCDI_ENTIRE_OFF 0xA4

#define LCDI_BIAS_17 0xA3
#define LCDI_BIAS_19  0xA2

#define LCDI_SHL_NORM 0xC0
#define LCDI_SHL_FLIP 0xC8

#define LCDI_PWRCTRL 0x28
#define LCDI_PWRCTRL_M 0x7

#define LCDI_BOOSTMODE 0xF8

#define LCDI_VREG 0x20
#define LCDI_VREG_M 0x7

#define LCDI_RMW 0xE0
#define LCDI_RMWEND 0xEE

#define LCDI_VOLUME 0x81

