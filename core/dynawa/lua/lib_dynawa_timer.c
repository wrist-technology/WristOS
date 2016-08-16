#include "lua.h"
#include "lauxlib.h"
#include "debug/trace.h"
#include "timer.h"

void lua_timer_handler(void *context);

static int l_start (lua_State *L) {
    TRACE_LUA("dynawa.timer.start\r\n");

    uint32_t when = luaL_checkint(L, 1);

    bool repeat;
    if (lua_isnoneornil(L, 2)) {
        repeat = false;
    } else {
        luaL_checktype(L, 2, LUA_TBOOLEAN);
        repeat = lua_toboolean(L, 2);
    }

    Timer *timer = malloc(sizeof(Timer));

    if (timer == NULL) {
        luaL_error(L, "no memory");
    }
    Timer_init(timer, 0);
    Timer_setHandler(timer, lua_timer_handler, timer);
    Timer_start(timer, when, repeat, true);

    //lua_pushnumber(L, (uint32_t)timer);
    lua_pushlightuserdata(L, (void *)timer);
    return 1;
}

static int l_cancel (lua_State *L) {
    TRACE_LUA("dynawa.timer.cancel\r\n");

    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);

    Timer *timer = (Timer*)lua_touserdata(L, 1);

    Timer_stop(timer);
    return 0;
}

static const struct luaL_reg timer [] = {
    {"start", l_start},
    {"cancel", l_cancel},
    {NULL, NULL}  /* sentinel */
};

int dynawa_timer_register (lua_State *L) {
    luaL_register(L, NULL, timer);
    return 1;
}
