/**
 * \file main.c
 * Main code
 *
 * Bluetooth wrist watch image r0.1a
 *
 * Author: Wrist Technology Ltd
 *
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "hardware_conf.h"
#include "firmware_conf.h"
#include "board_lowlevel.h"
#include <utils/macros.h>

#include <screen/screen.h>
#include <screen/font.h>

#include <debug/trace.h>
//#include <utils/interrupt_utils.h>
#include <utils/time.h>
#include <peripherals/serial.h>
#include <utils/delay.h>
#include <peripherals/pmc/pmc.h>
#include <peripherals/spi.h>
#include <utils/rprintf.h>

#include <utils/fastfixmath.h>

#include <peripherals/i2c.h>
//#include <peripherals/isl6295.h>
#include <microgui/ugscrdrv.h>
#include <microgui/microgui.h>

#include <sdcard/sdcard.h>
//#include <fat/fat.h>
#include <fatfs/ff.h>
#include "blconfgui.h"
#include "main.h"
#include "nvmrc.h"  //non volatile records - reset controller eeprom

//test:
//#include <peripherals/serial.h>
#include <utils/rscanf.h>



volatile AT91PS_PIO  pPIOA = AT91C_BASE_PIOA;     // PIO controller
volatile AT91PS_PIO  pPIOB = AT91C_BASE_PIOB;     // PIO controller

//scr_buf_t sram_screen[10];
scr_buf_t * scrbuf=NULL;

/**
 * The main function.
 *
 * \return Zero
 *
 */
 
/*
tugui_Btn btn1;
tugui_Btn btn2;
tugui_Btn btn21;
tugui_Btn btn22;
char lab1[] = "BTN1";
char lab2[] = "BTN2";
char lab21[] = "BTN21";
char lab22[] = "BTN22";
const char lab3[] = "LSTBOX";
const char lablab[] = "LSTBOX";

char *itemsx[]={"item1","item2","item3","item4","item5","item6"};

char *itemsSet1[]={"adf","item2","frg","ret","uuu5","item6"};
char *itemsSet2[]={"34","4322","7i8u","frr","eeee","kk88"};
char *itemsSet3[]={"dfg","tydh","fnmj","fhku","utu56","saweds"};

tugui_ListboxItems items=itemsx;
tugui_Listbox box,box2;

tugui_Label lab;
tugui_Win win;
tugui_Win win2;

//obj callbacks:

int btn1cb(void *p)
{
  TRACE_ALL("@btn1 callback ");
  return -123;
}

int btn2cb(void *p)
{
   TRACE_ALL("@btn2 callback ");
  uguiContextSwitch(1);
  return -123;
}

int btn21cb(void *p)
{
   TRACE_ALL("@btn21 callback ");
  uguiContextSwitch(0);
  return -123;
}

int boxDnCallback(void *p)
{
  //int *index = p; 
   //TRACE_ALL("@boxDn callback:%d | ",*index);
  if (box.itemi==0) items=itemsSet1;
  if (box.itemi==1) items=itemsSet2;
  if (box.itemi==2) items=itemsSet3;   
  box2.items=items;
  //refresh:
  uguiListbox(&box2, UGUI_EV_REDRAW);
  TRACE_ALL("@boxDn callback:"); 
  return -123;
}

int boxSelectCallback(void *p)
{
   TRACE_ALL("@box callback ");
  uguiContextSwitch(1);
  return -123;
}

void initGui(void);
*/



char strini[]="fooo";

int main (void)
{
    
    //scrWriteRect(0,0,20,80,0xffffff);
    scrbuf=NULL;
    initAIC();
    timer_init(); //re-enable ints
    delayms(100);

    dbg_usart_init();
    TRACE_ALL("\n\n");
    TRACE_ALL("Ints enabled.");
    scrWriteRect(0,0,159,127,0x003300);
    //===============================
    
    fontColor=0xffffff;
    fontBgColor=0;
    //fontCarridgeReturnPosX=0;
    
    fontSetCharPos(0,100);
    /*
    if ((r = f_mount (0, &fatfs)) == FR_OK) {
      //f_mount(0,&fatfs);
      if ((r=f_open(&fsfile,"boot",FA_WRITE))== FR_OK) 
        {
          if (r=f_write(&fsfile,buff,32,&bread)== FR_OK)
          {
            f_close(&fsfile);
          }  else TRACE_SCR("f_write err:%d",r);
        }   else TRACE_SCR("f_open err:%d",r);
    } else TRACE_SCR("f_mount err:%d",r);
    */  
    //buff[10]=0;
    //TRACE_SCR("%s",buff);  
  


    
    //int find_next(fatffdata_t * ff_data);
    
    fontSetCharPos(0,20);
    readSetupConf();
    
    scrrot=blconf.screenRotate;
    /*
    blconf.imageFile = strini;
    blconf.rstBtns[0]=1;blconf.rstBtns[1]=1;blconf.rstBtns[2] = 1;
    blconf.rstBtns[3]=0;blconf.rstBtns[4]=1;
    blconf.rstDelay=2;
    blconf.pwrBtns[0]=0;blconf.pwrBtns[1]=0;blconf.pwrBtns[2]=1; 
    blconf.pwrBtns[3]=0;blconf.pwrBtns[4]=0;
    blconf.pwrDelay=4;
    
    */
    
    blConfig();
    
    /*
    blConfGuiInit();
    
    
    uguiContextWrapper();
    uguiContextSwitch(0);
    delayms(3000);
    while(1)
    {
      if (!((pPIOB->PIO_PDSR)&(BUT2_MASK)))
      {
        uguiWriteEvt(200);
        delayms(200);
      }
      if (!((pPIOB->PIO_PDSR)&(BUT4_MASK)))
      {
        uguiWriteEvt(102);
        delayms(200);
      }
      if (!((pPIOB->PIO_PDSR)&(BUT0_MASK)))
      {
        uguiWriteEvt(201);
        delayms(200);
      }
      if (!((pPIOB->PIO_PDSR)&(BUT1_MASK)))
      {
        uguiWriteEvt(202);
        delayms(200);
      }
      uguiContextWrapper();
      delayms(50);
      TRACE_ALL("w");
    };
    */


}

#if 0
void initGui(void)
{
   btn1.x=1;btn1.y=1;btn1.w=40;btn1.h=15;
   btn1.label = lab1;
   btn1.releaseCallback=NULL;
   btn1.pushCallback= &btn1cb;
   btn1.pushEvent=100;
   btn1.releaseEvent=0;
   btn1.fgColor=0xff;
   btn1.bgColor=0xff0000;   
   
   btn2.x=60;btn2.y=1;btn2.w=40;btn2.h=15;
   btn2.label = lab2;
   btn2.releaseCallback=NULL;
   btn2.pushCallback= &btn2cb;
   btn2.pushEvent=102;
   btn2.releaseEvent=0;
   btn2.fgColor=0xff;
   btn2.bgColor=0xff0000; 
   
   btn21.x=100;btn21.y=60;btn21.w=40;btn21.h=15;
   btn21.label = lab21;
   btn21.releaseCallback=NULL;
   btn21.pushCallback= &btn21cb;
   btn21.pushEvent=102;
   btn21.releaseEvent=0; 
   btn21.fgColor=0xff00;
   btn21.bgColor=0xff0000; 
   
   btn22.x=100;btn22.y=80;btn22.w=40;btn22.h=15;
   btn22.label = lab22;
   btn22.releaseCallback=NULL;
   btn22.pushCallback= NULL;
   btn22.pushEvent=0;
   btn22.releaseEvent=0;
   btn22.fgColor=0xff00;
   btn22.bgColor=0xff0000; 
   
   box.x=0; box.y=30; //x,y pos of topleft corner in pix
   box.w=3; box.h=1; //width, length in items
   box.itemw=53;
   box.itemh=12; //width, length in pix
   box.label=lab3;
   box.items=items; //ptr to arry of charptr(strings)
   box.itemn=6; //no of items in listbox labels
   box.itemi=0;   
   box.flow=1;
   box.scrollDnEvent=200;
   box.scrollUpEvent=201;
   box.selectEvent=202;
   box.scrollDnCallback=&boxDnCallback;
   box.scrollUpCallback=&boxDnCallback;
   box.selectCallback=&boxSelectCallback; 
   box.fgColor=0xffffff;
   box.bgColor=0x00ff00;   
   box.fgColorSelected=0xffffff;
   box.bgColorSelected=0xaa;
   
   box2.x=63; box2.y=63; //x,y pos of topleft corner in pix
   box2.w=1; box2.h=4; //width, length in pix
   box2.itemw=60;
   box2.itemh=12; //width, length in pix
   box2.label=lab3;
   box2.items=items; //ptr to arry of charptr(strings)
   box2.itemn=6; //no of items in listbox labels
   box2.itemi=0;   
   box2.flow=0;
   box2.scrollDnEvent=200;
   box2.scrollUpEvent=201;
   box2.selectEvent=202;
   box2.scrollDnCallback=&boxDnCallback;
   box2.scrollUpCallback=NULL;
   box2.selectCallback=NULL; 
   box2.fgColor=0xffff22;
   box2.bgColor=0xa0bb00;   
   box2.fgColorSelected=0xffffff;
   box2.bgColorSelected=0xaa;
   
   lab.x = 20; lab.y=2;
   lab.w=40; lab.h=13;
   lab.label = lablab; lab.fgColor=0xffffff; lab.bgColor=0;
   
   //printf("hello.\n");
   
   win.x=0; win.y=0; win.w=127; win.h=150;
   win.objn=3;
   win.bgColor=0;
   win.objs[0].obj=&btn1;
   win.objs[0].evthandler=&uguiBtn;
   win.objs[1].obj=&btn2;
   win.objs[1].evthandler=&uguiBtn;
   win.objs[2].obj=&box;
   win.objs[2].evthandler=&uguiListbox;
   
   
   win2.x=1; win2.y=60; win2.w=140; win2.h=80;
   win2.objn=0;
   win2.bgColor=0xff0000;
   uguiWinAddObj(&win2,&btn21,&uguiBtn);
   uguiWinAddObj(&win2,&btn22,&uguiBtn);
   uguiWinAddObj(&win2,&box2,&uguiListbox);
   uguiWinAddObj(&win2,&lab,&uguiLabel);
   /*
   win2.objs[0].obj=&btn21;
   win2.objs[0].evthandler=&btn21;
   win2.objs[1].obj=&btn22;
   win2.objs[1].evthandler=&uguiBtn;
   win2.objs[2].obj=&box2;
   win2.objs[2].evthandler=&uguiListbox;
   win2.objs[3].obj=&lab;
   win2.objs[3].evthandler=&uguiLabel;
   */
   //
   uguiContextAddWin(win);
   uguiContextAddWin(win2);
   TRACE_ALL("GUI init done.");

}
#endif

/**
 * IO initialization
 *
 * Initialization for IO pins and ports and internal hardware.
 *
 */
#if 0
void init_lowlevel_io(void)
{
    volatile AT91PS_SMC2	pSMC = AT91C_BASE_SMC;
    volatile AT91PS_EBI  pEBI = AT91C_BASE_EBI;
    volatile AT91PS_UDP  pUDP = AT91C_BASE_UDP;
    
    // enable reset-button
    //*AT91C_RSTC_RMR = ( 0xA5000000 | AT91C_RSTC_URSTEN );
    
    *AT91C_PMC_PCER =   (1 << AT91C_ID_PIOA) |  // Enable Clock for PIO
                        (1 << AT91C_ID_IRQ0);  // Enable Clock for IRQ0

#if 1
    // set the POWER_ON I/O bit 
    SET(pPIOA->PIO_PER, PIN_POWER_ON);  // Enable PIO pin
    SET(pPIOA->PIO_OER, PIN_POWER_ON);  // Enable output
    SET(pPIOA->PIO_SODR, PIN_POWER_ON);  // Set Low to enable power
    
    SET(pPIOA->PIO_PER, PIN_LED);  // Enable PIO pin
    SET(pPIOA->PIO_OER, PIN_LED);  // Enable output
    SET(pPIOA->PIO_SODR, PIN_LED);  // Set Low to enable power
#else
    #warning Enable Power Pin!
#endif


    // disable USB pull-up 
    //SET(pPIO->PIO_PER, PIN_USB_PULLUP);   // Enable PIO pin
    //SET(pPIO->PIO_OER, PIN_USB_PULLUP);   // Enable outputs
    //SET(pPIO->PIO_SODR, PIN_USB_PULLUP);  // Set high

    // configure USB detection pin
    SET(pPIOB->PIO_PER, PIN_USB_DETECT);     // enable PIO pin
    SET(pPIOB->PIO_ODR, PIN_USB_DETECT);     // set pin as input
    SET(pPIOB->PIO_PPUER, PIN_USB_DETECT);   // enable pull-up

    // Enables the 48MHz USB clock UDPCK and System Peripheral USB Clock
    // Required to write to UDP_TXVC
    SET(pPMC->PMC_SCER, AT91C_PMC_UDP);
    SET(pPMC->PMC_PCER, (1 << AT91C_ID_UDP));
    
    SET(pUDP->UDP_TXVC, AT91C_UDP_TXVDIS);  // disable UDP tranceiver
                                            // because enabled by default after reset
    
    // Disables the 48MHz USB clock UDPCK and System Peripheral USB Clock
    // Save power
    //SET(pPMC->PMC_SCDR, AT91C_PMC_UDP);
    //SET(pPMC->PMC_PCDR, (1 << AT91C_ID_UDP));
    
    //extmem
    
    pEBI->EBI_CSA = 0x0;    
    pSMC->SMC2_CSR[0] = 0x0000B081;
    
}   // end of void init_lowlevel_io(void)
#endif