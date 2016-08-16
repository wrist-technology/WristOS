#include <stdio.h>
#include <stdlib.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "trace.h"
#include "led.h"

//#include <utils/macros.h>
#include <peripherals/spi.h>
#include <sdcard/sdcard.h>
//#include <fat/fat.h>
#include <ff.h>
#include "event.h"
#include "lib_dynawa_bt.h"

#define TEST  0    
#define LUA_LED 0 // !!! LED - PIO PA0 - AUDIO MUTE collision

extern bool lua_usb_msd_enabled;

static FATFS fatfs;
static FILINFO fileInfo;

void lua_timer_handler(void *context) {
    event ev;
    ev.type = EVENT_TIMER;
    ev.data.timer.type = EVENT_TIMER_FIRED;
    ev.data.timer.handle = (TimerHandle)context;

    TRACE_LUA("lua_timer_handler %x\r\n", context);
    event_post_isr(&ev);
}

int lua_event_loop (void) {

    TRACE_INFO("lua_event_loop %x\r\n", xTaskGetCurrentTaskHandle());

#if LUA_LED
    Io led;
    Led_init(&led);
    Led_setState(&led, 0);
#endif

    int error;

    FRESULT f;

    //no Task_sleep(10000);
    if ((f = disk_initialize (0)) != FR_OK) {
        f_printerror (f);
        TRACE_ERROR("disk_initialize\r\n");
        return 1;
    }
    if ((f = f_mount (0, &fatfs)) != FR_OK) {
        f_printerror (f);
        TRACE_ERROR("f_mount\r\n");
        return 1;
    }

    TRACE_LUA("luaL_newstate\r\n");
    lua_State *L = luaL_newstate();
    TRACE_LUA("done\r\n");
    if (L == NULL) {
        TRACE_ERROR("Err: luaL_newstate()\r\n");
        return 1;
    }
    TRACE_LUA("luaL_openlibs\r\n");
    luaL_openlibs(L);
    luaopen_dynawa(L);

    TRACE_LUA("done\r\n");

    unsigned long ticks = xTaskGetTickCount();

    if (luaL_loadfile(L, "_sys/boot.lua") || lua_pcall(L, 0, 0, 0)) {
        TRACE_ERROR("lua: %s\r\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        lua_close(L);
        panic("_sys/boot.lua");
    }

    ticks = xTaskGetTickCount() - ticks;
    TRACE_LUA("initialized %d\r\n", ticks);

    //fflush(stdout);
    while(1) {
        event ev;
#if TEST
        ev.type = EVENT_BUTTON_DOWN;
        ev.data.button.id = 1;
#else
        event_get(&ev, EVENT_WAIT_FOREVER);
#endif

/*
        int i;
        for(i = 0; i < sizeof(event); i++) {
            TRACE_LUA("ev[%d]=%d\r\n", i, *(((uint8_t*)&ev) + i));
        }
*/

        lua_getglobal(L, "handle_event");

        switch(ev.type) {
        case EVENT_BUTTON:
            switch(ev.data.button.type) {
            case EVENT_BUTTON_DOWN:
                TRACE_LUA("EVENT_BUTTON_DOWN %d\r\n", ev.data.button.id);
                lua_newtable(L);

                lua_pushstring(L, "type");
                lua_pushstring(L, "button_down");
                lua_settable(L, -3);

                lua_pushstring(L, "button");
                lua_pushnumber(L, ev.data.button.id);
                lua_settable(L, -3);
                break;
            case EVENT_BUTTON_HOLD:
                TRACE_LUA("EVENT_BUTTON_HOLD %d\r\n", ev.data.button.id);
                lua_newtable(L);

                lua_pushstring(L, "type");
                lua_pushstring(L, "button_hold");
                lua_settable(L, -3);

                lua_pushstring(L, "button");
                lua_pushnumber(L, ev.data.button.id);
                lua_settable(L, -3);
                break;
            case EVENT_BUTTON_UP:
                TRACE_LUA("EVENT_BUTTON_UP %d\r\n", ev.data.button.id);
                lua_newtable(L);

                lua_pushstring(L, "type");
                lua_pushstring(L, "button_up");
                lua_settable(L, -3);

                lua_pushstring(L, "button");
                lua_pushnumber(L, ev.data.button.id);
                lua_settable(L, -3);
                break;
            default:
                TRACE_ERROR("Uknown button event %x\r\n", ev.data.button.type);
            }
            break;
        case EVENT_TIMER:
            switch(ev.data.timer.type) {
            case EVENT_TIMER_FIRED:
                TRACE_LUA("EVENT_TIMER_FIRED %x\r\n", ev.data.timer.handle);

                {
                    Timer *timer = (Timer*)ev.data.timer.handle;
                    if (!timer->repeat && timer->freeOnStop) {
                        TRACE_LUA("timer freed\r\n");
                        
                        if (timer->magic != TIMER_MAGIC) {
                            panic("EVENT_TIMER_FIRED");
                        }
                        timer->magic = 0;
                        free(timer);
                    }
                }
                lua_newtable(L);

                lua_pushstring(L, "type");
                lua_pushstring(L, "timer_fired");
                lua_settable(L, -3);

                lua_pushstring(L, "handle");
                lua_pushlightuserdata(L, (void*)ev.data.timer.handle);
                lua_settable(L, -3);
                break;
            default:
                TRACE_ERROR("Uknown timer event %x\r\n", ev.data.timer.type);
            }
            break;
        case EVENT_BT:
            switch(ev.data.bt.type) {
            case EVENT_BT_ERROR:
                TRACE_INFO("EVENT_BT_ERROR\r\n");
                {
                    bt_event *btev = &ev.data.bt;
                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    bt_lua_socket *sock = (bt_lua_socket*)btev->sock;
                    lua_pushstring(L, "socket");
                    lua_rawgeti(L, LUA_REGISTRYINDEX, sock->ref_lua_socket);
                    lua_settable(L, -3);

                    lua_pushstring(L, "error");
                    lua_pushnumber(L, ev.data.bt.param.error);
                    lua_settable(L, -3);
                }
                break;
            case EVENT_BT_STARTED:
                TRACE_INFO("EVENT_BT_STARTED\r\n");
                lua_newtable(L);

                lua_pushstring(L, "type");
                lua_pushstring(L, "bluetooth");
                lua_settable(L, -3);

                lua_pushstring(L, "subtype");
                lua_pushnumber(L, ev.data.bt.type);
                lua_settable(L, -3);
                break;
            case EVENT_BT_STOPPED:
                TRACE_INFO("EVENT_BT_STOPPED\r\n");
                lua_newtable(L);

                lua_pushstring(L, "type");
                lua_pushstring(L, "bluetooth");
                lua_settable(L, -3);

                lua_pushstring(L, "subtype");
                lua_pushnumber(L, ev.data.bt.type);
                lua_settable(L, -3);
                break;
            case EVENT_BT_DATA:
                TRACE_INFO("EVENT_BT_DATA %x\r\n", ev.data.bt.sock);
                {
                    bt_event *btev = &ev.data.bt;

                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    bt_lua_socket *sock = (bt_lua_socket*)btev->sock;
                    lua_pushstring(L, "socket");
                    lua_rawgeti(L, LUA_REGISTRYINDEX, sock->ref_lua_socket);
                    lua_settable(L, -3);

                    /*
                    lua_pushstring(L, "handle");
                    lua_pushlightuserdata(L, (void*)btev->param.data.pcb);
                    lua_settable(L, -3);
                    */

                    lua_pushstring(L, "data");
                    struct pbuf *p = (struct pbuf *)btev->param.ptr;
                    uint16_t len = p->tot_len;

                    TRACE_INFO("EVENT_BT_DATA %d\r\n", len);
                    if (len) {
                        luaL_Buffer b;
                        luaL_buffinit(L, &b);  

                        // TODO: pbuf2buf()
                        int remain = len;
                        struct pbuf *q = p;
                        int count = 0;
                        while (remain) {
                            if (q == NULL) {
                                TRACE_ERROR("PBUF=NULL\r\n");
                                panic("EVENT_BT_DATA");
                                return;
                            }
                            int chunk_len = q->len;
                            TRACE_LUA("pbuf len %d\r\n", chunk_len);
                            int n = remain > chunk_len ? chunk_len : remain;
                            luaL_addlstring(&b, q->payload, n); 
                            remain -= n;
                            q = q->next;
                            count++;
                        }
                        luaL_pushresult(&b);
                    } else {
                        lua_pushnil(L);
                    }
                    lua_settable(L, -3);

                    pbuf_free(p);
                }
                break;
            case EVENT_BT_RFCOMM_CONNECTED:
                TRACE_INFO("EVENT_BT_RFCOMM_CONNECTED %x\r\n", ev.data.bt.sock);
                {
                    bt_event *btev = &ev.data.bt;

                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    bt_lua_socket *sock = (bt_lua_socket*)btev->sock;
                    lua_pushstring(L, "socket");
                    lua_rawgeti(L, LUA_REGISTRYINDEX, sock->ref_lua_socket);
                    lua_settable(L, -3);

                }
                break;
            case EVENT_BT_RFCOMM_ACCEPTED:
                TRACE_INFO("EVENT_BT_RFCOMM_ACCEPTED %x\r\n", ev.data.bt.sock);
                {
                    bt_event *btev = &ev.data.bt;

                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    bt_lua_socket *sock = (bt_lua_socket*)btev->sock;
                    lua_pushstring(L, "socket");
                    lua_rawgeti(L, LUA_REGISTRYINDEX, sock->ref_lua_socket);
                    lua_settable(L, -3);

                    bt_lua_socket *client_sock = (bt_lua_socket*)btev->param.ptr;
                    lua_pushstring(L, "client_socket");
                    lua_pushlightuserdata(L, (void*)client_sock);
                    lua_settable(L, -3);

                    lua_pushstring(L, "remote_bdaddr");
                    lua_pushlstring(L, &client_sock->sock.pcb->l2cappcb->remote_bdaddr, BT_BDADDR_LEN);
                    lua_settable(L, -3);
                }
                break;
            case EVENT_BT_RFCOMM_DISCONNECTED:
                TRACE_INFO("EVENT_BT_RFCOMM_DISCONNECTED %x\r\n", ev.data.bt.sock);
                {
                    bt_event *btev = &ev.data.bt;

                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    bt_lua_socket *sock = (bt_lua_socket*)btev->sock;
                    lua_pushstring(L, "socket");
                    lua_rawgeti(L, LUA_REGISTRYINDEX, sock->ref_lua_socket);
                    lua_settable(L, -3);

                    /*
                    lua_pushstring(L, "handle");
                    lua_pushlightuserdata(L, (void*)btev->param.data.pcb);
                    lua_settable(L, -3);
                    */
                }
                break;
            case EVENT_BT_LINK_KEY_NOT:
                TRACE_INFO("EVENT_BT_LINK_KEY_NOT\r\n");
                {
                    bt_event *btev = &ev.data.bt;

                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    struct bt_bdaddr_link_key *ev_bdaddr_link_key = btev->param.ptr;
                    lua_pushstring(L, "bdaddr");
                    lua_pushlstring(L, &ev_bdaddr_link_key->bdaddr, BT_BDADDR_LEN);
                    lua_settable(L, -3);

                    lua_pushstring(L, "link_key");
                    lua_pushlstring(L, &ev_bdaddr_link_key->link_key, BT_LINK_KEY_LEN); 
                    lua_settable(L, -3);

                    free(ev_bdaddr_link_key);
                }
                break;
            case EVENT_BT_LINK_KEY_REQ:
                TRACE_INFO("EVENT_BT_LINK_KEY_REQ\r\n");
                {
                    bt_event *btev = &ev.data.bt;

                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    struct bd_addr *ev_bdaddr = btev->param.ptr;
                    lua_pushstring(L, "bdaddr");
                    lua_pushlstring(L, ev_bdaddr, BT_BDADDR_LEN);
                    lua_settable(L, -3);

                    free(ev_bdaddr);
                }
                break;
            case EVENT_BT_FIND_SERVICE_RESULT:
                TRACE_INFO("EVENT_BT_FIND_SERVICE_RESULT %x\r\n", ev.data.bt.sock);
                {
                    bt_event *btev = &ev.data.bt;

                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    bt_lua_socket *sock = (bt_lua_socket*)btev->sock;
                    TRACE_INFO("ref_sock %x\r\n", sock->ref_lua_socket);
                    lua_pushstring(L, "socket");
                    lua_rawgeti(L, LUA_REGISTRYINDEX, sock->ref_lua_socket);
                    lua_settable(L, -3);

                    lua_pushstring(L, "channel");
                    lua_pushnumber(L, btev->param.service.cn);
                    lua_settable(L, -3);
                }
                break;
            case EVENT_BT_REMOTE_NAME:
                TRACE_INFO("EVENT_BT_REMOTE_NAME\r\n");
                {
                    bt_event *btev = &ev.data.bt;

                    lua_newtable(L);

                    lua_pushstring(L, "type");
                    lua_pushstring(L, "bluetooth");
                    lua_settable(L, -3);

                    lua_pushstring(L, "subtype");
                    lua_pushnumber(L, ev.data.bt.type);
                    lua_settable(L, -3);

                    struct bd_addr *ev_bdaddr = &btev->param.remote_name.bdaddr;
                    lua_pushstring(L, "bdaddr");
                    lua_pushlstring(L, ev_bdaddr, BT_BDADDR_LEN);
                    lua_settable(L, -3);

                    uint8_t *rname = btev->param.remote_name.name;
                    lua_pushstring(L, "name");
                    lua_pushstring(L, rname);
                    lua_settable(L, -3);

                    free(rname);
                }
                break;
            default:
                TRACE_ERROR("Uknown bt event %x\r\n", ev.data.bt.type);
            }
            break;
        case EVENT_BATTERY:
            TRACE_LUA("EVENT_BATTERY state %d\r\n", ev.data.battery.state);
            lua_newtable(L);

            lua_pushstring(L, "type");
            lua_pushstring(L, "battery");
            lua_settable(L, -3);

            lua_pushstring(L, "state");
            lua_pushnumber(L, ev.data.battery.state);
            lua_settable(L, -3);
            break;
        case EVENT_USB:
            TRACE_LUA("EVENT_USB state %d\r\n", ev.data.usb.state);

            if (ev.data.usb.state == EVENT_USB_CONNECTED && lua_usb_msd_enabled) {
                usb_msd(true);
            }

            lua_newtable(L);

            lua_pushstring(L, "type");
            lua_pushstring(L, "usb");
            lua_settable(L, -3);

            lua_pushstring(L, "state");
            lua_pushnumber(L, ev.data.usb.state);
            lua_settable(L, -3);
            break;
        case EVENT_ACCEL:
            TRACE_LUA("EVENT_ACCEL gesture %d\r\n", ev.data.accel.gesture);
            lua_newtable(L);

            lua_pushstring(L, "type");
            lua_pushstring(L, "accel");
            lua_settable(L, -3);

            lua_pushstring(L, "gesture");
            lua_pushnumber(L, ev.data.accel.gesture);
            lua_settable(L, -3);

            lua_pushstring(L, "x");
            lua_pushnumber(L, ev.data.accel.x);
            lua_settable(L, -3);

            lua_pushstring(L, "y");
            lua_pushnumber(L, ev.data.accel.y);
            lua_settable(L, -3);

            lua_pushstring(L, "z");
            lua_pushnumber(L, ev.data.accel.z);
            lua_settable(L, -3);
            break;
        case EVENT_AUDIO:
            TRACE_LUA("EVENT_AUDIO\r\n");
            lua_newtable(L);

            lua_pushstring(L, "type");
            lua_pushstring(L, "audio");
            lua_settable(L, -3);

            lua_pushstring(L, "subtype");
            lua_pushnumber(L, ev.data.audio.type);
            lua_settable(L, -3);

            if (ev.data.audio.type == EVENT_AUDIO_STOP) {
                uint32_t sample_ref =  ev.data.audio.data;
                lua_pushstring(L, "sample");
                lua_rawgeti(L, LUA_REGISTRYINDEX, sample_ref);
                lua_settable(L, -3);
                luaL_unref(L, LUA_REGISTRYINDEX, sample_ref);
                TRACE_INFO("EVENT_AUDIO_STOP %d\r\n", sample_ref);
            }
            break;
        default:
            TRACE_ERROR("Uknown event %x\r\n", ev.type);
        }

#if LUA_LED
        Led_setState(&led, 1);
#endif
        //unsigned long ticks = xTaskGetTickCount();
        unsigned long ticks = Timer_tick_count();

        //if (lua_pcall(L, #in, #out, err handler) != 0)
        error = lua_pcall(L, 1, 0, 0);
/*
        lua_call(L, 1, 0);
        error = 0;
*/

#if LUA_LED
        Led_setState(&led, 0);
#endif
        //ticks = xTaskGetTickCount() - ticks;
        ticks = Timer_tick_count() - ticks;
        TRACE_LUA("error %d %d\r\n", error, ticks);

        if (error) {
            TRACE_ERROR("lua: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
            panic("lua error");
        }
        //fflush(stdout);
    }
    lua_close(L);
    return 0;  
}

#if 0
char *my_gets (char *buff, int len) {
  
  char b[256];
  int p = 0;
  while(1) {
    int n = read(0, b, 256);

    if (n) {
      int i;
      for(i = 0; i < n; i++) {
        char c = b[i];
        TRACE_LUA("chr %d\r\n", c);
        buff[p++] = c; 
        if (c == '\r') {
          buff[p] = 0;
          return buff;
        }
      }
    }
  }
  return NULL;
}

int lua_main (void) {

  Io led;
  Led_init(&led);
  char buff[256];

  int fimage;
  uint32_t image_size;

/*
  void *x = malloc(10000000);
  TRACE_LUA("malloc %x\r\n", x);
  if (x) free(x);

  fprintf(stdout, "test %d\r\n", 1);
  gets(buff);
*/

  int error;

/*
  spi_init();
  Task_sleep(200);
  if ( sd_init() != SD_OK ) {
    TRACE_ERROR("SD card init failed!\r\n");
  }
*/

/*
  fat_init();

  fimage = fat_open("main.bin",_O_RDONLY);
  if (fimage!=-1)
  {
    image_size = fat_size(fimage);
    TRACE_LUA("main.bin:%dkB\n\r",image_size/1024);
  }
*/

  {
  FRESULT f;

  if ((f = disk_initialize (0)) != FR_OK) {
    f_printerror (f);
    TRACE_ERROR("disk_initialize\r\n");
  } else {
    ULONG p2;
    FATFS *fs;
    int res;
    DIR dir;
    f_mount (0, &fatfs);
/*
    if ((res = f_getfree ("", (ULONG *) &p2, &fs)))
    {
      TRACE_ERROR("f_getfree %d %s\r\n", res, f_ferrorlookup (res));
      f_printerror (res);
    } else {
      TRACE_LUA ("FAT type = %u\nBytes/Cluster = %u\nNumber of FATs = %u\n"
      "Root DIR entries = %u\nSectors/FAT = %u\nNumber of clusters = %u\n"
      "FAT start (lba) = %u\nDIR start (lba,clustor) = %u\nData start (lba) = %u\n",
      fs->fs_type, fs->sects_clust * 512, fs->n_fats,
      fs->n_rootdir, fs->sects_fat, fs->max_clust - 2,
      fs->fatbase, fs->dirbase, fs->database
      );
    }
*/
    if ((res = f_opendir (&dir, ""))) {
      TRACE_ERROR("f_opendir %d %s\r\n", res, f_ferrorlookup (res));
    } else {
  ULONG size;
  USHORT files;
  USHORT dirs;
  for (size = files = dirs = 0;;)
  {
    if (((res = f_readdir (&dir, &fileInfo)) != FR_OK) || !fileInfo.fname [0])
      break;

    if (fileInfo.fattrib & AM_DIR)
      dirs++;
    else
    {
      files++;
      size += fileInfo.fsize;
    }

    printf ("\n%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s",
        (fileInfo.fattrib & AM_DIR) ? 'D' : '-',
        (fileInfo.fattrib & AM_RDO) ? 'R' : '-',
        (fileInfo.fattrib & AM_HID) ? 'H' : '-',
        (fileInfo.fattrib & AM_SYS) ? 'S' : '-',
        (fileInfo.fattrib & AM_ARC) ? 'A' : '-',
        (fileInfo.fdate >> 9) + 1980, (fileInfo.fdate >> 5) & 15, fileInfo.fdate & 31,
        (fileInfo.ftime >> 11), (fileInfo.ftime >> 5) & 63,
        fileInfo.fsize, &(fileInfo.fname [0]));
  }

  TRACE_LUA ("\n%4u File(s),%10u bytes\n%4u Dir(s)", files, size, dirs);
    }
  }
  }
  TRACE_LUA("luaL_newstate\r\n");
  lua_State *L = luaL_newstate();
  TRACE_LUA("done\r\n");
  if (L == NULL) {
    TRACE_ERROR("Err: luaL_newstate()\r\n");
    return 1;
  }
  TRACE_LUA("luaL_openlibs\r\n");
  luaL_openlibs(L);

  TRACE_LUA("done\r\n");

  fflush(stdout);
  //while(read(0, buff, 10)) {
  //while(fgets(buff, sizeof(buff), stdin) != NULL) {
  //while(fgets(buff, sizeof(buff), stdin) != NULL) {
  while(my_gets(buff, sizeof(buff)) != NULL) {
    unsigned int ticks;
    TRACE_LUA("line <%s>\r\n", buff);

    Led_setState(&led, 1);
    ticks = xTaskGetTickCount();
    error = luaL_loadbuffer(L, buff, strlen(buff), "line") || lua_pcall(L, 0, 0, 0);
    ticks = xTaskGetTickCount() - ticks;
    Led_setState(&led, 0);
    TRACE_LUA("error %d %d\r\n", error, ticks);
    if (error) {
      fprintf(stderr, "%s", lua_tostring(L, -1));
      lua_pop(L, 1);
    }
    fflush(stdout);

    TRACE_LUA("fgets %s\r\n", buff);
  }
  lua_close(L);
  return 0;  
}
#endif


