#include "lua.h"
#include "lauxlib.h"
#include "debug/trace.h"
#include "png.h"
#include "bitmap.h"
#include "types.h"
#include "peripherals/oled/oled.h"

static int l_new (lua_State *L) {
    TRACE_LUA("dynawa.bitmap.new\r\n");

    uint16_t width = luaL_checkint(L, 1);
    uint16_t height = luaL_checkint(L, 2);

    uint8_t red = 0, green = 0, blue = 0, alpha = 0xff;

    if (!lua_isnoneornil(L, 3)) {
        red = luaL_checkint(L, 3);
        green = luaL_checkint(L, 4);
        blue = luaL_checkint(L, 5);
        if (!lua_isnoneornil(L, 6)) {
            alpha = luaL_checkint(L, 6);
        }
    }

    int bmp_size = bitmap_size(BMP_8RGBA, width, height);
    bitmap *bmp = (bitmap*)lua_newuserdata(L, bmp_size);
    bitmap_set_header(bmp, BMP_8RGBA, width, height);
    bitmap_set_pixels(bmp, 0, 0, bmp->header.width, bmp->header.height, red, green, blue, alpha);
    return 1;
}

static png_bytep c_alloc_png_rowbytes(png_infop info_ptr, uint32_t size, void *context) {
    TRACE_LUA("alloc_png_rowbytes %d\r\n", size);
    png_bytep rowbytes = malloc(size);
    *((png_bytep*)context) = rowbytes;
    return rowbytes;
}

typedef struct {
    lua_State *L;
    bitmap *bmp;
} png_alloc_rowbytes_context;

typedef struct {
    png_bytep png_data;
    uint32_t length;
    uint32_t offset;
} png_read_io_context;

static png_bytep alloc_png_rowbytes(png_infop info_ptr, uint32_t size, void *context) {
    png_alloc_rowbytes_context *lua_context = (png_alloc_rowbytes_context *)context;
    TRACE_LUA("alloc_png_rowbytes %d\r\n", size);
    bitmap *bmp = (bitmap*)lua_newuserdata(lua_context->L, sizeof(bitmap_header) + size);
    //png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
    bitmap_set_header(bmp, BMP_8RGBA, info_ptr->width, info_ptr->height);
    TRACE_LUA("bitmap %x\r\n", bmp);
    lua_context->bmp = bmp;
    return (uint8_t*)bmp + sizeof(bitmap_header);
}

static int l_from_png_file (lua_State *L) {
    TRACE_LUA("dynawa.bitmap.from_png_file\r\n");

    const char *filename = luaL_checkstring(L, 1);

    png_alloc_rowbytes_context alloc_context; 

    alloc_context.L = L;
    alloc_context.bmp = NULL;

    if(read_png(filename, alloc_png_rowbytes, &alloc_context, NULL, NULL)) {
        if(alloc_context.bmp) {
            lua_pop(L, 1);
        }
        lua_pushnil(L);
    }

    return 1;
}

static void png_read_data_fn(png_structp png_ptr, png_bytep data, png_size_t length) {
    png_read_io_context *read_io_context = (png_read_io_context*)png_get_io_ptr(png_ptr);
    
    TRACE_LUA("png_read_data_fn %d %d %d\r\n", read_io_context->offset, read_io_context->length, length);

    if (read_io_context->offset + length > read_io_context->length) {
        png_error(png_ptr, "dynawa.bitmap.from_png(): string too short");
    }
    memcpy(data, read_io_context->png_data + read_io_context->offset, length);

    read_io_context->offset += length;
}

static int l_from_png (lua_State *L) {
    TRACE_LUA("dynawa.bitmap.from_png\r\n");

    const char *png_data = luaL_checkstring(L, 1);

    png_alloc_rowbytes_context alloc_context; 

    alloc_context.L = L;
    alloc_context.bmp = NULL;

    png_read_io_context read_io_context; 

    read_io_context.png_data = png_data;
    read_io_context.length = lua_strlen(L, 1);
    read_io_context.offset = 0;

    if(read_png(NULL, alloc_png_rowbytes, &alloc_context, png_read_data_fn, &read_io_context)) {
        if(alloc_context.bmp) {
            lua_pop(L, 1);
        }
        lua_pushnil(L);
    }

    return 1;
}

static int l_copy (lua_State *L) {
    TRACE_LUA("dynawa.bitmap.copy\r\n");

    luaL_checktype(L, 1, LUA_TUSERDATA);
    bitmap *src_bmp = lua_touserdata(L, 1);

    int16_t x = luaL_checkint(L, 2);
    int16_t y = luaL_checkint(L, 3);
    uint16_t width = luaL_checkint(L, 4);
    uint16_t height = luaL_checkint(L, 5);

    uint32_t bmp_size = bitmap_size(BMP_8RGBA, width, height);
    bitmap *bmp = (bitmap*)lua_newuserdata(L, bmp_size);
    bitmap_set_header(bmp, BMP_8RGBA, width, height);

    bitmap_copy(bmp, src_bmp, x, y, width, height);
    return 1;
}

static int l_combine (lua_State *L) {
    TRACE_LUA("dynawa.bitmap.combine\r\n");

    luaL_checktype(L, 1, LUA_TUSERDATA);
    bitmap *bg_bmp = lua_touserdata(L, 1);

    luaL_checktype(L, 2, LUA_TUSERDATA);
    bitmap *ovl_bmp = lua_touserdata(L, 2);

    int16_t x = luaL_checkint(L, 3);
    int16_t y = luaL_checkint(L, 4);

    bool new_bitmap;
    if (lua_isnoneornil(L, 5)) {
        new_bitmap = false;
    } else {
        luaL_checktype(L, 5, LUA_TBOOLEAN);
        new_bitmap = lua_toboolean(L, 5);
    }
    bitmap *dst_bmp;
    if (new_bitmap) {
        TRACE_LUA("new bitmap\r\n");
        uint16_t width = bg_bmp->header.width;
        uint16_t height = bg_bmp->header.height;
        uint32_t bmp_size = bitmap_size(BMP_8RGBA, width, height);
        dst_bmp = (bitmap*)lua_newuserdata(L, bmp_size);
        bitmap_set_header(dst_bmp, BMP_8RGBA, width, height);
    } else {
        dst_bmp = bg_bmp;
        lua_pushvalue(L, 1);
    } 
    bitmap_combine(dst_bmp, bg_bmp, ovl_bmp, x, y);
    return 1;
}

static int l_mask (lua_State *L) {
    TRACE_LUA("dynawa.bitmap.mask\r\n");

    luaL_checktype(L, 1, LUA_TUSERDATA);
    bitmap *src_bmp = lua_touserdata(L, 1);

    luaL_checktype(L, 2, LUA_TUSERDATA);
    bitmap *msk_bmp = lua_touserdata(L, 2);

    int16_t x = luaL_checkint(L, 3);
    int16_t y = luaL_checkint(L, 4);

    uint16_t width = msk_bmp->header.width;
    uint16_t height = msk_bmp->header.height;
    uint32_t bmp_size = bitmap_size(BMP_8RGBA, width, height);
    bitmap *dst_bmp = (bitmap*)lua_newuserdata(L, bmp_size);
    bitmap_set_header(dst_bmp, BMP_8RGBA, width, height);

// TODO check bitmap_mask()
    bitmap_mask(dst_bmp, src_bmp, msk_bmp, x, y);
    return 1;
}

static int l_show (lua_State *L) {
    TRACE_LUA("dynawa.bitmap.show\r\n");

    luaL_checktype(L, 1, LUA_TUSERDATA);
    bitmap *bmp = lua_touserdata(L, 1);

    bool rotate;
    if (lua_isnoneornil(L, 2)) {
        rotate = false;
    } else {
        luaL_checktype(L, 2, LUA_TBOOLEAN);
        rotate = lua_toboolean(L, 2);
    }
    scrLock();
// TODO rotate flag
    scrWriteBitmapRGBA(0, 0, OLED_RESOLUTION_X - 1, OLED_RESOLUTION_Y - 1, ((uint8_t*)bmp + sizeof(bitmap_header)));
    //scrWriteBitmapRGBA2(0, 0, 0, 0, OLED_RESOLUTION_X, OLED_RESOLUTION_Y, bmp);
    scrUnLock();
    return 0;
}

static int l_show_partial (lua_State *L) {
    TRACE_LUA("dynawa.bitmap.show_partial\r\n");

    luaL_checktype(L, 1, LUA_TUSERDATA);
    bitmap *bmp = lua_touserdata(L, 1);

    int16_t x;
    if (lua_isnoneornil(L, 2)) {
        x = 0;
    } else {
        x = luaL_checkint(L, 2);
    }
    int16_t y;
    if (lua_isnoneornil(L, 3)) {
        y = 0;
    } else {
        y = luaL_checkint(L, 3);
    }
    uint16_t width;
    if (lua_isnoneornil(L, 4)) {
        width = bmp->header.width;
    } else {
        width = luaL_checkint(L, 4);
    }
    uint16_t height;
    if (lua_isnoneornil(L, 5)) {
        height = bmp->header.height;
    } else {
        height = luaL_checkint(L, 5);
    }
    int16_t scr_x = luaL_checkint(L, 6);
    int16_t scr_y = luaL_checkint(L, 7);

    bool rotate;
    if (lua_isnoneornil(L, 8)) {
        rotate = false;
    } else {
        luaL_checktype(L, 8, LUA_TBOOLEAN);
        rotate = lua_toboolean(L, 8);
    }
    scrLock();
// TODO rotate flag
    scrWriteBitmapRGBA2(scr_x, scr_y, x, y, width, height, bmp);
    scrUnLock();
    return 0;
}

static const struct luaL_reg f_bitmap[] = {
    {"new", l_new},
    {"from_png_file", l_from_png_file},
    {"from_png", l_from_png},
    {"copy", l_copy},
    {"combine", l_combine},
    {"mask", l_mask},
    {"show", l_show},
    {"show_partial", l_show_partial},
    {NULL, NULL}  /* sentinel */
};

int dynawa_bitmap_register (lua_State *L) {
    luaL_register(L, NULL, f_bitmap);
    return 1;
}
