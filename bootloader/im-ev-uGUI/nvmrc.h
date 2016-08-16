/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#define I2CRC_PHY_ADDR (0xFE>>1)

#define BLCONF_POST_ADDR 0x10
#define BLCONF_MISC_ADDR 0x11
#define BLCONF_POST_SRAM (1<<0)
#define BLCONF_POST_GG   (1<<1)
#define BLCONF_POST_LED  (1<<2)
#define BLCONF_POST_RTC  (1<<3)
#define BLCONF_POST_ACCEL (1<<4)

#define BLCONF_MISC_SCRROT (1<<0)
#define BLCONF_MISC_MSDBOOT (1<<1)

#define RC_FOFFON 0x2
#define RC_PCKEY 0xC5

#define RC_REG_RSTBTN 0x04
#define RC_REG_PWRBTN 0x05
#define RC_REG_PCKEY 0x85
#define RC_REG_PWRCTRL 0x84

extern tBLConf blconf;

extern void readSetupConf(void);
extern void resetByRstCtrl(void);
extern void writeSetupConf(void);

extern int16_t getBatVoltage(void);


