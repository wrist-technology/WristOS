#include "lua.h"
#include "lauxlib.h"
#include "debug/trace.h"
#include "types.h"
#include "ff.h"
#include <stdlib.h>

#define MAX_PATH_LEN    255

// Not reentrant but who cares
static char path_buff[MAX_PATH_LEN + 1];

static char *remove_trailing_slash(const char *path) {
/* removing possible trailing separator */
    int len = strlen(path);
    if (len > 1 && (len--, (path[len] == '/' || path[len] == '\\'))) {
/* TODO: check "   /" case */
        if (len > MAX_PATH_LEN) {
            return NULL;
        }
        strncpy(path_buff, path, len);
        path_buff[len] = '\0';
        return path_buff;
    } else {
        return path;
    }
}

static int l_mkdir (lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    TRACE_LUA("dynawa.file.mkdir(%s)\r\n", path);

    path = remove_trailing_slash(path);
    if (path == NULL) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (_mkdir(path, 0)) {
        //if (errno == EEXIST)
        lua_pushboolean(L, false);
        /* else
           lua_pushstring(L, "mkdir");
           lua_error(L);
           */
    } else {
        lua_pushboolean(L, true);
    }
    return 1;
}

static WCHAR lfn_buff[_MAX_LFN + 1];
static FILINFO file_info = {
    0, 0, 0, 0, "", lfn_buff, _MAX_LFN + 1    
};

static int l_dir_stat (lua_State *L) {

    const char *path = luaL_checkstring(L, 1);

    TRACE_LUA("dynawa.file.dir_stat(%s)\r\n", path);

    path = remove_trailing_slash(path);
    if (path == NULL) {
        lua_pushnil(L);
        return 1;
    }

    int res;
    DIR dir;
    if ((res = f_opendir (&dir, path))) {
        TRACE_ERROR("f_opendir %d %s\r\n", res, f_ferrorlookup (res));
        lua_pushfstring(L, "f_opendir %d %s", res, f_ferrorlookup (res));
        lua_error(L);
        //lua_pushnil(L); 
    } else {
        /* MV
        file_info.lfname = lfn_buff;
        file_info.lfsize = _MAX_LFN + 1;
        */

        lua_newtable(L);
        while(true) {
            //FILINFO file_info;
            if (((res = f_readdir (&dir, &file_info)) != FR_OK) || !file_info.fname [0])
                break;

//MV TODO: unicode support (16b)
            char *fname = (file_info.lfname && file_info.lfname [0]) ? file_info.lfname : file_info.fname;

            TRACE_LUA("dir entry %s\r\n", fname);
            lua_pushstring(L, fname);
            if (file_info.fattrib & AM_DIR) {
                lua_pushstring(L, "dir");
            } else {
                lua_pushnumber(L, file_info.fsize);
            }
            lua_settable(L, -3);
        }
    }


    return 1;
}

static const struct luaL_reg file [] = {
    {"mkdir", l_mkdir},
    {"dir_stat", l_dir_stat},
    {NULL, NULL}  /* sentinel */
};

int dynawa_file_register (lua_State *L) {
    luaL_register(L, NULL, file);
    return 1;
}
