/* ========================================================================== */
/*                                                                            */
/*   Filename.c                                                               */
/*   (c) 2001 Author                                                          */
/*                                                                            */
/*   Description                                                              */
/*                                                                            */
/* ========================================================================== */

#include <unistd.h>

//bitmap type dependent:
#include "hardware_conf.h"
#include "firmware_conf.h"
#include <screen/screen.h>

#include <microgui/ugscrdrv.h>
#include <microgui/microgui.h>
#include "embedgraphics.h"

const tugui_Bitmap micro_arrow_up[5*6]={0xffffff,0xffffff,0       ,0xffffff,0xffffff,
                                        0xffffff,0,       0       ,0       ,0xffffff,
                                        0       ,0,       0       ,0       ,0       ,
                                        0xffffff,0xffffff,0       ,0xffffff,0xffffff,
                                        0xffffff,0xffffff,0       ,0xffffff,0xffffff,
                                        0xffffff,0xffffff,0       ,0xffffff,0xffffff,
                                       };

const tugui_Bitmap micro_arrow_dn[5*6]={0xffffff,0xffffff,0       ,0xffffff,0xffffff,
                                        0xffffff,0xffffff,0       ,0xffffff,0xffffff,
                                        0xffffff,0xffffff,0       ,0xffffff,0xffffff,                                                                                
                                        0       ,0,       0       ,0       ,0       ,
                                        0xffffff,0,       0       ,0       ,0xffffff,
                                        0xffffff,0xffffff,0       ,0xffffff,0xffffff,
                                       };
                                       
const tugui_Bitmap micro_M[5*6]=       {0       ,0xffffff,0xffffff,0xffffff,0,
                                        0       ,0,       0xffffff,0       ,0,
                                        0       ,0xffffff,0       ,0xffffff,0,
                                        0       ,0xffffff,0xffffff,0xffffff,0,
                                        0       ,0xffffff,0xffffff,0xffffff,0,
                                        0       ,0xffffff,0xffffff,0xffffff,0,
                                       };
const tugui_Bitmap micro_Enter[5*6]=   {0xffffff,0xffffff,0xffffff,0xffffff,0,
                                        0xffffff,0xffffff,0       ,0xffffff,0,
                                        0xffffff,0       ,0       ,0xffffff,0,
                                        0       ,0       ,0       ,0       ,0,
                                        0xffffff,0       ,0       ,0xffffff,0xffffff,
                                        0xffffff,0xffffff,0       ,0xffffff,0xffffff,
                                       };     
                                       
const tugui_Bitmap micro_menu[5*6]={    0xffffff,0       ,0       ,0       ,0,
                                        0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,
                                        0xffffff,0,       0       ,0       ,0       ,
                                        0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,
                                        0xffffff,0,       0       ,0       ,0       ,
                                        0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,
                                       };                                                                                                             

//       X
//      XXX
//     XXXXX
//       X
//       X
//       X 
