#include "lua.h"
#include "lauxlib.h"
#include "audio.h"
#include "debug/trace.h"
#include "event.h"

static void* audio_stop_callback(void *arg) {
    event ev;
    ev.type = EVENT_AUDIO;
    ev.data.audio.type = EVENT_AUDIO_STOP;
    ev.data.audio.data = arg;
    event_post_isr(&ev);
}

static int l_play (lua_State *L) {

    luaL_checktype(L, 1, LUA_TUSERDATA);
    audio_sample *sample = lua_touserdata(L, 1);
    lua_pushvalue(L, 1);
    uint32_t ref_sample = luaL_ref(L, LUA_REGISTRYINDEX);

    uint32_t sample_rate;
    if (lua_isnoneornil(L, 2)) {
        sample_rate = 0;
    } else {
        sample_rate = luaL_checkint(L, 2);
    }
    int32_t loop;
    if (lua_isnoneornil(L, 3)) {
        loop = 0;
    } else {
        loop = luaL_checkint(L, 3);
    }

    TRACE_LUA("dynawa.audio.play(%d, %d, %d)\r\n", ref_sample, sample_rate, loop);

    audio_play(sample, 0, 0, audio_stop_callback, (void*)ref_sample);

    return 0;
}

static int l_stop (lua_State *L) {

    TRACE_LUA("dynawa.audio.stop()\r\n");

    audio_stop();

    return 0;
}

static void *lua_malloc(size_t size, void *arg) {
    lua_State *L = (lua_State *)arg;
    return lua_newuserdata(L, size);
}

static int l_sample_from_wav_file (lua_State *L) {

    const char *path = luaL_checkstring(L, 1);

    int8_t volume;
    if (lua_isnoneornil(L, 2)) {
        volume = -1;
    } else {
        volume = luaL_checkint(L, 2);
    }
    uint32_t sample_rate;
    if (lua_isnoneornil(L, 3)) {
        sample_rate = 0;
    } else {
        sample_rate = luaL_checkint(L, 3);
    }
    int32_t loop;
    if (lua_isnoneornil(L, 4)) {
        loop = 0;
    } else {
        loop = luaL_checkint(L, 4);
    }

    TRACE_LUA("dynawa.audio.sample_from_wav_file(%s, %d, %d, %d)\r\n", path, volume, sample_rate, loop);

    audio_sample *sample = audio_sample_from_wav_file(path, volume, sample_rate, loop, lua_malloc, L);

    return 1;
}

static const struct luaL_reg audio [] = {
    {"play", l_play},
    {"stop", l_stop},
    {"sample_from_wav_file", l_sample_from_wav_file},
    {NULL, NULL}  /* sentinel */
};

int dynawa_audio_register (lua_State *L) {
    luaL_register(L, NULL, audio);
    return 1;
}
