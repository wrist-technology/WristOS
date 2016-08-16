/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

typedef struct 
{  
  uint8_t rstBtns[5];
  uint8_t rstDelay;
  uint8_t pwrBtns[5];
  uint8_t pwrDelay;
  uint8_t POST_sram;
  uint8_t POST_gasgauge;
  uint8_t POST_led;
  uint8_t POST_rtc;
  uint8_t POST_accel;
  uint8_t screenRotate;
  uint8_t defMSDBoot;

} tBLConf;

extern void blConfGuiInit(void);

extern void blConfig(void);

extern char bootname[32];
extern tBLConf blconf;
