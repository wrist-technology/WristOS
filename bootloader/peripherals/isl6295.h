/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#define I2CGG_PHY_ADDR (0x26>>1)
//#define I2CGG_PHY_ADDR (0x26)

#define I2CGG_REG_OPmode 0x07A
#define I2CGG_BANK_OPmode 0x01

#define I2CGG_BANK_GPIOctrl 0x01
#define I2CGG_REG_GPIOctrl 0x53

#define I2CGG_POR_BITMASK 0x02
#define I2CGG_SHELF_BITMASK (0x04)

#define I2CGG_BANK_VPctrl 0x01
#define I2CGG_REG_VPctrl 0x4E

#define I2CGG_BANK_Ictrl 0x01
#define I2CGG_REG_Ictrl 0x42
#define I2CGG_DEFVAL_Ictrl 0xF0 

#define I2CGG_BANK_ITctrl 0x01
#define I2CGG_REG_ITctrl 0x46
#define I2CGG_DEFVAL_ITctrl 0xF9 

#define I2CGG_BANK_ADconfig 0x01
#define I2CGG_REG_ADconfig 0x43

#define I2CGG_BANK_VPres 0x01
#define I2CGG_REG_VPres 0x4C

#define I2CGG_BANK_Ires 0x01
#define I2CGG_REG_Ires 0x40

//#define I2CGG_DEFVAL_VPctrl 0xFB
//#define I2CGG_DEFVAL_VPctrl 0xAB
#define I2CGG_DEFVAL_VPctrl 0x9B


