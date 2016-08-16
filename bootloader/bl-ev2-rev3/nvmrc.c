/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */



#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "hardware_conf.h"
#include "firmware_conf.h"
#include <screen/screen.h>
#include <screen/font.h>
#include <debug/trace.h>
#include <microgui/ugscrdrv.h>
#include <microgui/microgui.h>
#include <peripherals/oled/oled.h>
#include <peripherals/i2c.h>
#include <peripherals/isl6295.h>
#include "blconfgui.h"
#include "nvmrc.h" 

tBLConf blconf;
uint32_t hwUID;

void readSetupConf(void)
{
  uint8_t i,b;
  TRACE_ALL("R NVM:");
  i2cMasterConf(I2CRC_PHY_ADDR, 1, 0x00, I2CMASTER_READ);
  //TRACE_SCR("id %x",i2cReadByte());
  
	i2cMultipleReadByteStart();
	
	hwUID=0;
	hwUID|= ((uint32_t)i2cMultipleReadByteRead())<<24;TRACE_ALL("."); 
	hwUID|= ((uint32_t)i2cMultipleReadByteRead())<<16;
	hwUID|= ((uint32_t)i2cMultipleReadByteRead())<<8; 
	hwUID|= ((uint32_t)i2cMultipleReadByteEnd()); 
	TRACE_ALL("U%x",hwUID);
	
	
	i2cMasterConf(I2CRC_PHY_ADDR, 1, RC_REG_RSTBTN, I2CMASTER_READ);
	//reset btn:
	
	b= i2cReadByte(); TRACE_ALL("R:%x",b);
	blconf.rstBtns[0]=b&0x1;
	blconf.rstBtns[1]=0xff;
	blconf.rstBtns[2]=(b>>1)&0x1;
	blconf.rstBtns[3]=(b>>2)&0x1;
	blconf.rstBtns[4]=(b>>3)&0x1;
	blconf.rstDelay = (b>>4)&0x7;
	
  i2cMasterConf(I2CRC_PHY_ADDR, 1, RC_REG_PWRBTN, I2CMASTER_READ);	
	//pwr btn
	b= i2cReadByte();TRACE_ALL("P:%x",b);
	blconf.pwrBtns[0]=b&0x1;
	blconf.pwrBtns[1]=0xff;
	blconf.pwrBtns[2]=(b>>1)&0x1;
	blconf.pwrBtns[3]=(b>>2)&0x1;
	blconf.pwrBtns[4]=(b>>3)&0x1;
	blconf.pwrDelay = (b>>4)&0x7;
  
  //bl configuration: POST
  i2cMasterConf(I2CRC_PHY_ADDR, 1, BLCONF_POST_ADDR, I2CMASTER_READ);	
	//pwr btn
	b= i2cReadByte();

  if (b&BLCONF_POST_SRAM) blconf.POST_sram=1; else blconf.POST_sram=0;
  if (b&BLCONF_POST_GG) blconf.POST_gasgauge=1; else blconf.POST_gasgauge=0;
  if (b&BLCONF_POST_LED) blconf.POST_led=1; else blconf.POST_led=0;
  if (b&BLCONF_POST_RTC) blconf.POST_rtc=1; else blconf.POST_rtc=0;
  if (b&BLCONF_POST_ACCEL) blconf.POST_accel=1; else blconf.POST_accel=0;   
  
  //bl configuration: POST
  i2cMasterConf(I2CRC_PHY_ADDR, 1, BLCONF_MISC_ADDR, I2CMASTER_READ);	
	//pwr btn
	b= i2cReadByte();
  if (b&BLCONF_MISC_SCRROT) blconf.screenRotate=1; else blconf.screenRotate=0;
  if (b&BLCONF_MISC_MSDBOOT) blconf.defMSDBoot=1; else blconf.defMSDBoot=0; 
  
}

void writeSetupConf(void)
{
  uint8_t b;
  b=0;
  if (blconf.POST_sram) b|=BLCONF_POST_SRAM; 
  if (blconf.POST_gasgauge) b|=BLCONF_POST_GG;
  if (blconf.POST_led) b|=BLCONF_POST_LED;
  if (blconf.POST_rtc) b|=BLCONF_POST_RTC;
  if (blconf.POST_accel) b|=BLCONF_POST_ACCEL;
  
  i2cMasterConf(I2CRC_PHY_ADDR, 1, BLCONF_POST_ADDR, I2CMASTER_WRITE);
	//post config:
	(void)i2cWriteByte(b);
	
	//rst btn:
	b=0;
	if (blconf.rstBtns[0]==1) b|=0x1;	
	if (blconf.rstBtns[2]==1) b|=(1<<1);
	if (blconf.rstBtns[3]==1) b|=(1<<2);
	if (blconf.rstBtns[4]==1) b|=(1<<3);
	b|= (blconf.rstDelay&0x7)<<4;	
	i2cMasterConf(I2CRC_PHY_ADDR, 1, RC_REG_RSTBTN, I2CMASTER_WRITE);	
	(void)i2cWriteByte(b);
	
	//pwr btn:
	b=0;
	if (blconf.pwrBtns[0]==1) b|=0x1;	
	if (blconf.pwrBtns[2]==1) b|=(1<<1);
	if (blconf.pwrBtns[3]==1) b|=(1<<2);
	if (blconf.pwrBtns[4]==1) b|=(1<<3);
	b|= (blconf.pwrDelay&0x7)<<4;	
	i2cMasterConf(I2CRC_PHY_ADDR, 1, RC_REG_PWRBTN, I2CMASTER_WRITE);	
	(void)i2cWriteByte(b);
	
	//misc:
	b=0;
	if (blconf.screenRotate==1) b|=0x1;	
	if (blconf.defMSDBoot==1) b|=(1<<1);
	i2cMasterConf(I2CRC_PHY_ADDR, 1, BLCONF_MISC_ADDR, I2CMASTER_WRITE);	
	(void)i2cWriteByte(b);
}

void resetByRstCtrl(void)
{
  i2cMasterConf(I2CRC_PHY_ADDR, 1, RC_REG_PCKEY, I2CMASTER_WRITE);
	//reset btn:	
	(void)i2cWriteByte(RC_PCKEY);
	
  i2cMasterConf(I2CRC_PHY_ADDR, 1, RC_REG_PWRCTRL, I2CMASTER_WRITE);
	//reset btn:
	
	(void)i2cWriteByte(RC_FOFFON);
}

uint32_t getUID(void)
{
  return hwUID;
}

int16_t getBatVoltage(void)
{
   uint8_t b;
   uint16_t t;
    
   i2cMasterConf(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_VPres<<10)|(I2CGG_REG_VPres), I2CMASTER_READ);
    b=i2cReadByte();//TRACE_ALL("[%x]",b);
    t = (uint16_t)b;
    i2cMasterConf(I2CGG_PHY_ADDR, 2, (I2CGG_BANK_VPres<<10)|(I2CGG_REG_VPres+1), I2CMASTER_READ);
    b=i2cReadByte();//TRACE_ALL("[%x]",b);
    t |= (((uint16_t)b)<<8);
    
   return ((t>>6)*199)/10;  
}
