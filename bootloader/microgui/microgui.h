
/* Micro GUI 
 *
 *
 *
 *
 */
 
 
#ifndef _MICROGUI_H_
#define _MICROGUI_H_

//mandatory events----
#define UGUI_EV_REDRAW 0x1


//lenght in power of 2 (2^3 = 8)
#define UGUI_EVT_BUF_LEN 3

#define UGUI_MAXWINS 10
#define UGUI_MAXWINOBJS 16

#define UGUI_WINATTR_DRAWFRAME (1<<0)
 
#include "scroutput.h"

#ifndef UGD_STDOUT 
#include <inttypes.h> 
#endif

typedef tugd_Coord tugui_ScrCoord;
typedef tugd_Font tugui_Font;
typedef tugd_Color tugui_Color;
typedef uint32_t tugui_Event;
typedef uint32_t tugui_Err;
typedef int (*tugui_Callback)(const void*);

typedef  tugd_Bitmap tugui_Bitmap;
//typedef tugui_CallbackT *tugui_Callback;

//Listbox:
typedef  char** tugui_ListboxItems; 
//corret static declaration: char *itemsx[]={"item1","item2","item3"}; 

typedef  uint8_t tugui_Context;

//general object handler (non-object solution)
//only one possible way to call event_handler(obj_data_struct*, evt)
typedef struct
{  
   void * obj; //ptr to data.structure
   tugui_Event (*evthandler)(const void *, tugui_Event); //object event handler e.g. uguiBtn()   
} tugui_Obj;
//tugui_Component;


   
typedef tugui_Obj tugui_Objects[UGUI_MAXWINOBJS];

typedef struct 
{
  tugui_ScrCoord x,y;
} tugui_Plain;

// simple button structure:
typedef struct 
{
  tugui_ScrCoord x,y; //x,y pos of topleft corner in pix
  tugui_ScrCoord w,h; //width, length in pix
  char* label; 
  tugui_Event pushEvent; //event to push the button
  tugui_Event releaseEvent; //event to relase button  
  tugui_Callback pushCallback;  //fuction called after press.
  tugui_Callback releaseCallback;  //fuction called after press.
  tugui_Font font;
  tugui_Color fgColor;
  tugui_Color bgColor;
  tugui_Color fgColorPushed;
  tugui_Color bgColorPushed;
  tugui_Bitmap *bitmap;
  tugui_ScrCoord bitmapw,bitmaph;
} tugui_Btn;

// simple label structure (no events):
typedef struct 
{
  tugui_ScrCoord x,y; //x,y pos of topleft corner in pix
  tugui_ScrCoord w,h; //width, length in pix
  char* label;
  tugui_Font font;
  tugui_Color fgColor;
  tugui_Color bgColor;
} tugui_Label;

// simple image structure (no events):
typedef struct 
{
  tugui_ScrCoord x,y; //x,y pos of topleft corner in pix
  tugui_ScrCoord w,h; //width, length in pix 
  tugui_Bitmap *bitmap;
} tugui_Image;

typedef struct 
{
  tugui_ScrCoord x,y; //x,y pos of topleft corner in pix
  tugui_ScrCoord len,thick; //width, length in pix  
  uint8_t vert;
  tugui_Color fgColor;  
} tugui_Ruller;

// simple listbox/nosub menus structure:
typedef struct 
{
  tugui_ScrCoord x,y; //x,y pos of topleft corner in pix
  tugui_ScrCoord w,h; //width, length in pix
  tugui_ScrCoord itemw,itemh; //width, length in pix
  tugui_ScrCoord propw,proph;
  char* label;
  tugui_ListboxItems *items; //ptr to arry of charptr(strings)
   tugui_ListboxItems *props; //properties
  uint16_t itemn; //no of items in listbox labels
  uint16_t itemi; 
  //TODO tugui_ListboxFlow flow;
  uint8_t flow;
  tugui_Event scrollDnEvent;
  tugui_Event scrollUpEvent;
  tugui_Event selectEvent;
  tugui_Callback scrollDnCallback;
  tugui_Callback scrollUpCallback;
  tugui_Callback selectCallback; 
  tugui_Font font;
  tugui_Color fgColor;
  tugui_Color bgColor;
  tugui_Color fgColorSelected;
  tugui_Color bgColorSelected;
} tugui_Listbox;

typedef struct
{
  tugui_ScrCoord x,y; //x,y pos of topleft corner in pix
  tugui_ScrCoord w,h; //width, length in pix
  int16_t min, max, value;
  uint8_t vert, mirror; //vertical/horizontal
  tugui_Color fgColor;
  tugui_Color bgColor;
} tugui_Bargraph;

//win container:
typedef struct 
{
  tugui_ScrCoord x,y; //x,y pos of topleft corner in pix
  tugui_ScrCoord w,h; //width, length in pix
  tugui_Objects objs; //
  uint8_t objn;
  tugui_Color bgColor;
  tugui_Color fgColor;
  uint8_t attr;  
} tugui_Win;
#endif


extern tugui_Event uguiBtn(tugui_Btn *btn, tugui_Event evt);
extern tugui_Event uguiRuller(tugui_Ruller *rul, tugui_Event evt);
extern tugui_Event uguiLabel(tugui_Label *lab, tugui_Event evt);
extern tugui_Event uguiImage(tugui_Image *img, tugui_Event evt);
extern tugui_Event uguiListbox(tugui_Listbox *box, tugui_Event evt);
extern tugui_Event uguiWin(tugui_Win *win, tugui_Event evt);
extern tugui_Err uguiWinAddObj(tugui_Win *win,  void* obj,   tugui_Event (*evthandler)(const void *, tugui_Event));
extern tugui_Event uguiBargraph(tugui_Bargraph *bar, tugui_Event evt); 

extern void uguiContextWrapper(void);
extern tugui_Err uguiContextSwitch(tugui_Context c);
extern tugui_Err uguiContextAddWin(tugui_Win *win);
extern void uguiWriteEvt(tugui_Event evt);

extern void uguiInitContext(void);

extern tugui_Context uguiLastContext(void);

extern tugui_Context uguiContext(void); //get context



