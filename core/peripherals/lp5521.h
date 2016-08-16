#ifndef LP5521_H_
#define LP5521_H_

#define LEDRGB_PHY_ADDR (0x64 >> 1)

//register ENABLE
#define LEDRGB_REG_ENABLE 0x00
//bits/flags:
#define LEDRGB_CHIPEN 0x40


//register OP MODE
#define LEDRGB_REG_OPMODE 0x01
//bits/flags:
#define LEDRGB_RMODE_DC (0x3<<4)
#define LEDRGB_GMODE_DC (0x3<<2)
#define LEDRGB_BMODE_DC (0x3<<0)

//registers R/G/B PWM
#define LEDRGB_REG_RPWM 0x02
#define LEDRGB_REG_GPWM 0x03
#define LEDRGB_REG_BPWM 0x04

//registers R/G/B Current
#define LEDRGB_REG_RCURRENT 0x05
#define LEDRGB_REG_GCURRENT 0x06
#define LEDRGB_REG_BCURRENT 0x07


//register CONFIG
#define LEDRGB_REG_CONFIG 0x08
//bits
#define LEDRGB_CPMODE_AUTO (0x3<<3)
#define LEDRGB_PWM_HF (0x1<<6)
#define LEDRGB_PWRSAVE_EN (0x1<<5)
#define LEDRGB_R_TO_BATT (0x1<<2)
#define LEDRGB_CLK_DET_EN (0x1<<1) 
#define LEDRGB_INT_CLK_EN (0x1<<0)

#endif // LP5521_H_
