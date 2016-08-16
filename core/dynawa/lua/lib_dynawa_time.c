#include "lua.h"
#include "lauxlib.h"
#include "debug/trace.h"

#define CFG_PM

static int l_set (lua_State *L) {

    uint32_t now = luaL_checkint(L, 1);

    TRACE_LUA("dynawa.time.set(%d)\r\n", now);

    rtc_open();
    rtc_set_epoch_seconds(now);
    rtc_close();

    return 0;
}

static int l_get (lua_State *L) {
    TRACE_LUA("dynawa.time.get\r\n");

    unsigned int seconds, milliseconds;

    rtc_open();
    seconds = rtc_get_epoch_seconds (&milliseconds);
    rtc_close();

    lua_pushnumber(L, seconds);
    lua_pushnumber(L, milliseconds);
    return 2;
}

static int l_milliseconds (lua_State *L) {
    TRACE_LUA("dynawa.time.milliseconds\r\n");

#ifdef CFG_PM
    lua_pushnumber(L, Timer_tick_count());
#else
    lua_pushnumber(L, xTaskGetTickCount());
#endif
    return 1;
}

static const struct luaL_reg time [] = {
    {"set", l_set},
    {"get", l_get},
    {"milliseconds", l_milliseconds},
    {NULL, NULL}  /* sentinel */
};

int dynawa_time_register (lua_State *L) {
    luaL_register(L, NULL, time);
    return 1;
}
