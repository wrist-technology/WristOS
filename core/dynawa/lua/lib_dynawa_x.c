#include "lua.h"
#include "lauxlib.h"
#include "analogin.h"
#include "gasgauge.h"
#include "debug/trace.h"

static int l_adc (lua_State *L) {

    uint32_t ch = luaL_checkint(L, 1);

    TRACE_LUA("dynawa.x.adc(%d)\r\n", ch);

    AnalogIn adc;
    AnalogIn_init(&adc, ch);
    int value = AnalogIn_value(&adc);
    //int value = AnalogIn_valueWait(&adc);
    AnalogIn_close(&adc);

    lua_pushnumber(L, value);
    return 1;
}

static int l_display_power (lua_State *L) {

    uint32_t state = luaL_checkint(L, 1);

    TRACE_LUA("dynawa.x.display_power(%d)\r\n", state);

    scrLock();
    int result = display_power(state);
    scrUnLock();

    lua_pushnumber(L, result);
    return 1;
}

static int l_display_brightness (lua_State *L) {

    uint32_t level = luaL_checkint(L, 1);

    TRACE_LUA("dynawa.x.display_brightness(%d)\r\n", level);

    scrLock();
    int result = display_brightness(level);
    scrUnLock();

    lua_pushnumber(L, result);
    return 1;
}

static int l_vibrator_set (lua_State *L) {

    luaL_checktype(L, 1, LUA_TBOOLEAN);
    bool on = lua_toboolean(L, 1);
    //bool on = luaL_checkint(L, 1);

    TRACE_LUA("dynawa.x.vibrator_set(%d)\r\n", on);

    vibrator_set(on);

    return 0;
}

static int l_battery_stats (lua_State *L) {

    TRACE_LUA("dynawa.x.battery_stats()\r\n");

    gasgauge_stats stats;
    //gasgauge_get_stats (&stats);
    battery_get_stats (&stats);

    lua_newtable(L);

    lua_pushstring(L, "state");
    lua_pushnumber(L, stats.state);
    lua_settable(L, -3);

    lua_pushstring(L, "voltage");
    lua_pushnumber(L, stats.voltage);
    lua_settable(L, -3);

    lua_pushstring(L, "current");
    lua_pushnumber(L, stats.current);
    lua_settable(L, -3);

    return 1;
}

static int l_accel_stats (lua_State *L) {

    TRACE_LUA("dynawa.x.accel_stats()\r\n");

    int16_t x = 0, y = 0, z = 0;
    accel_read(&x, &y, &z, true);

    lua_newtable(L);

    lua_pushstring(L, "x");
    lua_pushnumber(L, x);
    lua_settable(L, -3);

    lua_pushstring(L, "y");
    lua_pushnumber(L, y);
    lua_settable(L, -3);

    lua_pushstring(L, "z");
    lua_pushnumber(L, z);
    lua_settable(L, -3);

    return 1;
}

extern uint32_t total_time_in_sleep;
extern uint32_t total_time_in_deep_sleep;
extern bool usb_state_connected;
extern char __heap_end__[];

static int l_sys_stats (lua_State *L) {

    TRACE_LUA("dynawa.x.sys_stats()\r\n");

    lua_newtable(L);

    lua_pushstring(L, "sbrk");
    lua_pushnumber(L, __heap_end__ - (char*)sbrk(0));
    lua_settable(L, -3);

    lua_pushstring(L, "up_time");
    lua_pushnumber(L, Timer_tick_count());
    lua_settable(L, -3);

#ifdef CFG_PM
    lua_pushstring(L, "time_sleep");
    lua_pushnumber(L, total_time_in_sleep);
    lua_settable(L, -3);

#ifdef CFG_DEEP_SLEEP
    lua_pushstring(L, "time_deep_sleep");
    lua_pushnumber(L, total_time_in_deep_sleep);
    lua_settable(L, -3);
#endif
#endif

    lua_pushstring(L, "usb_connected");
    lua_pushnumber(L, usb_state_connected);
    lua_settable(L, -3);

    return 1;
}

bool lua_usb_msd_enabled = false;

static int l_usb_mode (lua_State *L) {

    uint32_t mode = luaL_checkint(L, 1);

    TRACE_LUA("dynawa.x.usb_mode(%d)\r\n", mode);

    lua_usb_msd_enabled = mode;

    return 0;
}

static int l_pm_sd (lua_State *L) {

    luaL_checktype(L, 1, LUA_TBOOLEAN);
    bool on = lua_toboolean(L, 1);
    TRACE_LUA("dynawa.x.pm_sd(%d)\r\n", on);

    sd_set_pm(on);

    return 0;
}

static const struct luaL_reg x [] = {
    {"adc", l_adc},
    {"display_power", l_display_power},
    {"display_brightness", l_display_brightness},
    {"vibrator_set", l_vibrator_set},
    {"battery_stats", l_battery_stats},
    {"accel_stats", l_accel_stats},
    {"sys_stats", l_sys_stats},
    {"usb_mode", l_usb_mode},
    {"pm_sd", l_pm_sd},
    {NULL, NULL}  /* sentinel */
};

int dynawa_x_register (lua_State *L) {
    luaL_register(L, NULL, x);
    return 1;
}
