#include "bitmap.h"
#include "debug/trace.h"
#include "types.h"

#define BITMAP_RGBA_A(rgba)     (((rgba) >> 24) & 0xff)

uint32_t bitmap_size(bitmap_type type, uint16_t width, uint16_t height) {
    return sizeof(bitmap_header) + (width * height * 4);
}

void bitmap_set_header(bitmap *bmp, bitmap_type type, uint16_t width, uint16_t height) {
    bmp->header.type = type;
    bmp->header.width = width;
    bmp->header.height = height;
}

bool bitmap_check_bounds(bitmap *bmp, int16_t *x, int16_t *y, uint16_t *width, uint16_t *height) {
    if (*x + *width < 0 || *y + *width < 0 || *x >= bmp->header.width || *y >= bmp->header.height) {
        TRACE_BMP("out of bounds\r\n");
        return false;
    }
    if (*x < 0) {
        *width += *x;
        *x = 0;
        TRACE_BMP("width reduced %d\r\n", *width);
    }
    if (*y < 0) {
        *height += *y;
        *y = 0;
        TRACE_BMP("height reduced %d\r\n", *height);
    }
    if (*x + *width > bmp->header.width) {
        *width = bmp->header.width - *x;
        TRACE_BMP("width reduced %d\r\n", *width);
    }
    if (*y + *height > bmp->header.height) {
        *height = bmp->header.height - *y;
        TRACE_BMP("height reduced %d\r\n", *height);
    }
    return true;
}

void bitmap_set_pixels(bitmap *bmp, int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {

    TRACE_BMP("bitmap_set_pixels %x %d %d %d %d rgba %x %x %x %x\r\n", bmp, x, y, width, height, red, green, blue, alpha);

    if(!bitmap_check_bounds(bmp, &x, &y, &width, &height))
        return;

    //uint32_t filler = (uint32_t)red | ((uint32_t)green << 8) | ((uint32_t)blue << 16) | ((uint32_t)alpha << 24);
    uint32_t filler = red | (green << 8) | (blue << 16) | (alpha << 24);

    int num_pixels = width * height;

    uint32_t *pixels = (uint32_t*)((uint8_t*)bmp + sizeof(bitmap_header));

    int index = y * bmp->header.width + x;
    int delta = bmp->header.width - width;
    int i;
    for (i = 0; i < height; i++) {
        int j;
        for (j = 0; j < width; j++) {
            pixels[index++] = filler;
        }
        index += delta;
    }
}

void bitmap_copy2(bitmap *dst_bmp, int16_t dst_x, int16_t dst_y, bitmap *src_bmp, int16_t src_x, int16_t src_y, uint16_t width, uint16_t height) {
    TRACE_BMP("bitmap_copy2 dst %x [%d %d] src %x [%d %d] %d %d\r\n", dst_bmp, dst_x, dst_y, src_bmp, src_x, src_y, width, height);

    if(!bitmap_check_bounds(src_bmp, &src_x, &src_y, &width, &height))
        return;
    if(!bitmap_check_bounds(dst_bmp, &dst_x, &dst_y, &width, &height))
        return;


    uint32_t *src_pixels = (uint32_t*)((uint8_t*)src_bmp + sizeof(bitmap_header));
    uint32_t *dst_pixels = (uint32_t*)((uint8_t*)dst_bmp + sizeof(bitmap_header));

    int src_index = src_y * src_bmp->header.width + src_x;
    int src_delta = src_bmp->header.width - width;

    int dst_index = dst_y * dst_bmp->header.width + dst_x;
    int dst_delta = dst_bmp->header.width - width;

    int i;
    for (i = 0; i < height; i++) {
        int j;
        for (j = 0; j < width; j++) {
            dst_pixels[dst_index++] = src_pixels[src_index++];
        }
        src_index += src_delta;
        dst_index += dst_delta;
    }
}

uint8_t bit_mask[] = {
    0x80,
    0x40,
    0x20,
    0x10,
    0x08,
    0x04,
    0x02,
    0x01,
};

void bitmap_op_combine(bitmap *dst_bmp, bitmap *src1_bmp, int16_t dst_x, int16_t dst_y, bitmap *src_bmp, int16_t src_x, int16_t src_y, uint16_t width, uint16_t height) {
    TRACE_BMP("bitmap_op_combine dst %x %x [%d %d] src %x [%d %d] %d %d\r\n", dst_bmp, src1_bmp, dst_x, dst_y, src_bmp, src_x, src_y, width, height);

    if (dst_bmp->header.width != src1_bmp->header.width || dst_bmp->header.height != src1_bmp->header.height) {
        TRACE_BMP("bitmap diff size\r\n");
        return;
    }

    if(!bitmap_check_bounds(src_bmp, &src_x, &src_y, &width, &height))
        return;
    if(!bitmap_check_bounds(dst_bmp, &dst_x, &dst_y, &width, &height))
        return;



    if (0) {
        uint32_t *dst_pixels = (uint32_t*)((uint8_t*)dst_bmp + sizeof(bitmap_header));
        uint32_t *src1_pixels = (uint32_t*)((uint8_t*)src1_bmp + sizeof(bitmap_header));
        uint8_t *src_pixels = ((uint8_t*)src_bmp + sizeof(bitmap_header));

        uint32_t fg_rgba = 0xffffffff;
        uint32_t bg_rgba = 0xff000000;

        int src_index = (src_y * src_bmp->header.width + src_x) / 8;
        int src_delta = (src_bmp->header.width - width) / 8 + ((width % 8) > 0);
        int src_bit = src_x % 8;

        int dst_index = dst_y * dst_bmp->header.width + dst_x;
        int dst_delta = dst_bmp->header.width - width;

        int i;
        for (i = 0; i < height; i++) {
            int j;
            int bit = src_bit;
            uint8_t src_byte = src_pixels[src_index];
            for (j = 0; j < width; j++) {
                //bool src_opaque = src_byte & (1 << (7 - bit_offset));
                bool src_opaque = src_byte & bit_mask[bit];

                // TODO : alpha blending
                dst_pixels[dst_index] = src_opaque ? fg_rgba : src1_pixels[dst_index];
                //dst_pixels[dst_index] = src_opaque ? fg_rgba : bg_rgba;
                bit++;
                if (bit > 7) {
                    bit = 0;
                    src_byte = src_pixels[++src_index];
                }
                dst_index++;
            }
            src_index += src_delta;
            dst_index += dst_delta;
        }
    } else {
        uint32_t *dst_pixels = (uint32_t*)((uint8_t*)dst_bmp + sizeof(bitmap_header));
        uint32_t *src1_pixels = (uint32_t*)((uint8_t*)src1_bmp + sizeof(bitmap_header));
        uint32_t *src_pixels = (uint32_t*)((uint8_t*)src_bmp + sizeof(bitmap_header));
        int src_index = src_y * src_bmp->header.width + src_x;
        int src_delta = src_bmp->header.width - width;

        int dst_index = dst_y * dst_bmp->header.width + dst_x;
        int dst_delta = dst_bmp->header.width - width;

        int i;
        for (i = 0; i < height; i++) {
            int j;
            for (j = 0; j < width; j++) {
                uint32_t src_pixel = src_pixels[src_index];
                uint8_t src_alpha = BITMAP_RGBA_A(src_pixel);

                // TODO : alpha blending
                dst_pixels[dst_index] = src_alpha ? src_pixel : src1_pixels[dst_index];
                src_index++;
                dst_index++;
            }
            src_index += src_delta;
            dst_index += dst_delta;
        }
    }
}

void bitmap_op_mask(bitmap *dst_bmp, bitmap *src1_bmp, int16_t dst_x, int16_t dst_y, bitmap *src_bmp, int16_t src_x, int16_t src_y, uint16_t width, uint16_t height) {
    TRACE_BMP("bitmap_op_mask dst %x %x [%d %d] src %x [%d %d] %d %d\r\n", dst_bmp, src1_bmp, dst_x, dst_y, src_bmp, src_x, src_y, width, height);

    if (dst_bmp->header.width != src1_bmp->header.width || dst_bmp->header.height != src1_bmp->header.height) {
        TRACE_BMP("bitmap diff size\r\n");
        return;
    }

    if(!bitmap_check_bounds(src_bmp, &src_x, &src_y, &width, &height))
        return;
    if(!bitmap_check_bounds(dst_bmp, &dst_x, &dst_y, &width, &height))
        return;


    uint32_t *dst_pixels = (uint32_t*)((uint8_t*)dst_bmp + sizeof(bitmap_header));
    uint32_t *src1_pixels = (uint32_t*)((uint8_t*)src1_bmp + sizeof(bitmap_header));
    uint32_t *src_pixels = (uint32_t*)((uint8_t*)src_bmp + sizeof(bitmap_header));

    int src_index = src_y * src_bmp->header.width + src_x;
    int src_delta = src_bmp->header.width - width;

    int dst_index = dst_y * dst_bmp->header.width + dst_x;
    int dst_delta = dst_bmp->header.width - width;

    int i;
    for (i = 0; i < height; i++) {
        int j;
        for (j = 0; j < width; j++) {
            // dst_pixels[dst_index] = src_pixels[src_index];
            // dst_pixels[dst_index] = src1_pixels[dst_index];
            dst_pixels[dst_index] = (src1_pixels[dst_index] & 0xff000000) | (src_pixels[src_index] & 0x00ffffff);
            src_index++;
            dst_index++;
        }
        src_index += src_delta;
        dst_index += dst_delta;
    }
}

void bitmap_copy(bitmap *dst_bmp, bitmap *src_bmp, int16_t src_x, int16_t src_y, uint16_t dst_width, uint16_t dst_height) {

    TRACE_BMP("bitmap_copy dst %x src %x [%d %d] %d %d\r\n", dst_bmp, src_bmp, src_x, src_y, dst_width, dst_height);
    int16_t d_x1 = 0;
    int16_t d_y1 = 0;
    int16_t d_x2 = d_x1 + dst_width - 1;
    int16_t d_y2 = d_y1 + dst_height - 1;

    int16_t s_x1 = -src_x;
    int16_t s_y1 = -src_y;
    int16_t s_x2 = s_x1 + src_bmp->header.width - 1;
    int16_t s_y2 = s_y1 + src_bmp->header.height - 1;

    TRACE_BMP("src [%d %d] [%d %d]\r\n", s_x1, s_y1, s_x2, s_y2);
    TRACE_BMP("dst [%d %d] [%d %d]\r\n", d_x1, d_y1, d_x2, d_y2);
    //if (s_y1 > 0) {
    if (d_y1 < s_y1) {
        int16_t y2 = s_y1 < d_y2 ? s_y1 - 1 : d_y2;
        TRACE_BMP("clear(T) 0 0 %d %d\r\n", d_x2, y2); 
        uint16_t w = d_x2 + 1;
        uint16_t h = y2 + 1;
        bitmap_set_pixels(dst_bmp, 0, 0, w, h, 0, 0, 0, 0);
    }
    if (d_y1 <= s_y2 && d_y2 >= s_y1) {

        int16_t y1 = s_y1 < 0 ? 0 : s_y1;
        int16_t y2 = s_y2 < d_y2 ? s_y2 : d_y2;

        //if (s_x1 > 0) {
        if (d_x1 < s_x1) {
            int16_t x2 = s_x1 < d_x2 ? s_x1 - 1 : d_x2;
            TRACE_BMP("clear(L) 0 %d %d %d\r\n", y1, x2, y2); 
            uint16_t w = x2 + 1;
            uint16_t h = y2 - y1 + 1;
            bitmap_set_pixels(dst_bmp, 0, y1, w, h, 0, 0, 0, 0);
        }
        
        if (d_x1 <= s_x2 && d_x2 >= s_x1) {
            int16_t x1 = s_x1 < 0 ? 0 : s_x1;
            int16_t x2 = s_x2 < d_x2 ? s_x2 : d_x2;
            //TRACE_BMP("copy %d %d %d %d\r\n", x1, y1, x2, y2); 
            uint16_t w = x2 - x1 + 1;
            uint16_t h = y2 - y1 + 1;
            TRACE_BMP("copy dst [%d %d] src [%d %d] %d %d\r\n", x1, y1, (src_x < 0 ? 0 : src_x), (src_y < 0 ? 0 : src_y), w, h); 
            bitmap_copy2(dst_bmp, x1, y1, src_bmp, (src_x < 0 ? 0 : src_x), (src_y < 0 ? 0 : src_y), w, h); 
        }

        if (d_x2 > s_x2) {
            int16_t x1 = s_x2 < 0 ? 0 : s_x2 + 1;
            TRACE_BMP("clear(R) %d %d %d %d\r\n", x1, y1, d_x2, y2); 
            uint16_t w = d_x2 - x1 + 1;
            uint16_t h = y2 - y1 + 1;
            bitmap_set_pixels(dst_bmp, x1, y1, w, h, 0, 0, 0, 0);
        }
    }
    if (d_y2 > s_y2) {
        int16_t y1 = s_y2 < 0 ? 0 : s_y2 + 1;
        TRACE_BMP("clear(B) 0 %d %d %d\r\n", y1, d_x2, d_y2); 
        uint16_t w = d_x2 + 1;
        uint16_t h = d_y2 - y1 + 1;
        bitmap_set_pixels(dst_bmp, 0, y1, w, h, 0, 0, 0, 0);
    }
}

void bitmap_combine(bitmap *dst_bmp, bitmap *bg_bmp, bitmap *ovl_bmp, int16_t src_x, int16_t src_y) {
    TRACE_BMP("bitmap_combine dst %x bg %x ovl %x [%d %d]\r\n", dst_bmp, bg_bmp, ovl_bmp, src_x, src_y);
    bool dst_ne_bg = dst_bmp != bg_bmp;

    int16_t s_x1 = src_x;
    int16_t s_y1 = src_y;
    int16_t s_x2 = s_x1 + ovl_bmp->header.width - 1;
    int16_t s_y2 = s_y1 + ovl_bmp->header.height - 1;

    int16_t d_x1 = 0;
    int16_t d_y1 = 0;
    int16_t d_x2 = d_x1 + dst_bmp->header.width - 1;
    int16_t d_y2 = d_y1 + dst_bmp->header.height - 1;

    TRACE_BMP("src [%d %d] [%d %d]\r\n", s_x1, s_y1, s_x2, s_y2);
    TRACE_BMP("dst [%d %d] [%d %d]\r\n", d_x1, d_y1, d_x2, d_y2);
    //if (s_y1 > 0) {
    if (dst_ne_bg && d_y1 < s_y1) {
        int16_t y2 = s_y1 < d_y2 ? s_y1 - 1 : d_y2;
        TRACE_BMP("clear(T) 0 0 %d %d\r\n", d_x2, y2); 
        uint16_t w = d_x2 + 1;
        uint16_t h = y2 + 1;
        //bitmap_set_pixels(dst_bmp, 0, 0, w, h, 0, 0, 0, 0);
        bitmap_copy2(dst_bmp, 0, 0, bg_bmp, 0, 0, w, h); 
    }
    if (d_y1 <= s_y2 && d_y2 >= s_y1) {

        int16_t y1 = s_y1 < 0 ? 0 : s_y1;
        int16_t y2 = s_y2 < d_y2 ? s_y2 : d_y2;

        //if (s_x1 > 0) {
        if (dst_ne_bg && d_x1 < s_x1) {
            int16_t x2 = s_x1 < d_x2 ? s_x1 - 1 : d_x2;
            TRACE_BMP("clear(L) 0 %d %d %d\r\n", y1, x2, y2); 
            uint16_t w = x2 + 1;
            uint16_t h = y2 - y1 + 1;
            //bitmap_set_pixels(dst_bmp, 0, y1, w, h, 0, 0, 0, 0);
            bitmap_copy2(dst_bmp, 0, y1, bg_bmp, 0, y1, w, h); 
        }
        
        if (d_x1 <= s_x2 && d_x2 >= s_x1) {
            int16_t x1 = s_x1 < 0 ? 0 : s_x1;
            int16_t x2 = s_x2 < d_x2 ? s_x2 : d_x2;
            //TRACE_BMP("copy %d %d %d %d\r\n", x1, y1, x2, y2); 
            uint16_t w = x2 - x1 + 1;
            uint16_t h = y2 - y1 + 1;
            TRACE_BMP("copy dst [%d %d] src [%d %d] %d %d\r\n", x1, y1, (src_x < 0 ? -src_x : 0), (src_y < 0 ? -src_y : 0), w, h); 

// MV src_x/y handled differently than in copy()
            //bitmap_copy2(dst_bmp, x1, y1, src_bmp, (src_x < 0 ? 0 : src_x), (src_y < 0 ? 0 : src_y), w, h); 
            // TODO combine src with dst
            //bitmap_copy2(dst_bmp, x1, y1, ovl_bmp, (src_x < 0 ? -src_x : 0), (src_y < 0 ? -src_y : 0), w, h); 
            bitmap_op_combine(dst_bmp, bg_bmp, x1, y1, ovl_bmp, (src_x < 0 ? -src_x : 0), (src_y < 0 ? -src_y : 0), w, h); 
        }

        if (dst_ne_bg && d_x2 > s_x2) {
            int16_t x1 = s_x2 < 0 ? 0 : s_x2 + 1;
            TRACE_BMP("clear(R) %d %d %d %d\r\n", x1, y1, d_x2, y2); 
            uint16_t w = d_x2 - x1 + 1;
            uint16_t h = y2 - y1 + 1;
            //bitmap_set_pixels(dst_bmp, x1, y1, w, h, 0, 0, 0, 0);
            bitmap_copy2(dst_bmp, x1, y1, bg_bmp, x1, y1, w, h); 
        }
    }
    if (dst_ne_bg && d_y2 > s_y2) {
        int16_t y1 = s_y2 < 0 ? 0 : s_y2 + 1;
        TRACE_BMP("clear(B) 0 %d %d %d\r\n", y1, d_x2, d_y2); 
        uint16_t w = d_x2 + 1;
        uint16_t h = d_y2 - y1 + 1;
        //bitmap_set_pixels(dst_bmp, 0, y1, w, h, 0, 0, 0, 0);
        bitmap_copy2(dst_bmp, 0, y1, bg_bmp, 0, y1, w, h); 
    }
}

void bitmap_mask(bitmap *dst_bmp, bitmap *src_bmp, bitmap *msk_bmp, int16_t src_x, int16_t src_y) {
    TRACE_BMP("bitmap_mask dst %x src %x msk %x [%d %d]\r\n", dst_bmp, src_bmp, msk_bmp, src_x, src_y);

    int16_t d_x1 = 0;
    int16_t d_y1 = 0;
    int16_t d_x2 = d_x1 + dst_bmp->header.width - 1;
    int16_t d_y2 = d_y2 + dst_bmp->header.height - 1;

    int16_t s_x1 = -src_x;
    int16_t s_y1 = -src_y;
    int16_t s_x2 = s_x1 + src_bmp->header.width - 1;
    int16_t s_y2 = s_y1 + src_bmp->header.height - 1;

    TRACE_BMP("src [%d %d] [%d %d]\r\n", s_x1, s_y1, s_x2, s_y2);
    TRACE_BMP("dst [%d %d] [%d %d]\r\n", d_x1, d_y1, d_x2, d_y2);
    //if (s_y1 > 0) {
    if (d_y1 < s_y1) {
        int16_t y2 = s_y1 < d_y2 ? s_y1 - 1 : d_y2;
        TRACE_BMP("clear(T) 0 0 %d %d\r\n", d_x2, y2); 
        uint16_t w = d_x2 + 1;
        uint16_t h = y2 + 1;
        bitmap_set_pixels(dst_bmp, 0, 0, w, h, 0, 0, 0, 0);
    }
    if (d_y1 <= s_y2 && d_y2 >= s_y1) {

        int16_t y1 = s_y1 < 0 ? 0 : s_y1;
        int16_t y2 = s_y2 < d_y2 ? s_y2 : d_y2;

        //if (s_x1 > 0) {
        if (d_x1 < s_x1) {
            int16_t x2 = s_x1 < d_x2 ? s_x1 - 1 : d_x2;
            TRACE_BMP("clear(L) 0 %d %d %d\r\n", y1, x2, y2); 
            uint16_t w = x2 + 1;
            uint16_t h = y2 - y1 + 1;
            bitmap_set_pixels(dst_bmp, 0, y1, w, h, 0, 0, 0, 0);
        }
        
        if (d_x1 <= s_x2 && d_x2 >= s_x1) {
            int16_t x1 = s_x1 < 0 ? 0 : s_x1;
            int16_t x2 = s_x2 < d_x2 ? s_x2 : d_x2;
            //TRACE_BMP("copy %d %d %d %d\r\n", x1, y1, x2, y2); 
            uint16_t w = x2 - x1 + 1;
            uint16_t h = y2 - y1 + 1;
            TRACE_BMP("copy dst [%d %d] src [%d %d] %d %d\r\n", x1, y1, (src_x < 0 ? 0 : src_x), (src_y < 0 ? 0 : src_y), w, h); 
            // TODO mask src using msk 
            //bitmap_copy2(dst_bmp, x1, y1, src_bmp, (src_x < 0 ? 0 : src_x), (src_y < 0 ? 0 : src_y), w, h); 
            bitmap_op_mask(dst_bmp, msk_bmp, x1, y1, src_bmp, (src_x < 0 ? 0 : src_x), (src_y < 0 ? 0 : src_y), w, h); 
        }

        if (d_x2 > s_x2) {
            int16_t x1 = s_x2 < 0 ? 0 : s_x2 + 1;
            TRACE_BMP("clear(R) %d %d %d %d\r\n", x1, y1, d_x2, y2); 
            uint16_t w = d_x2 - x1 + 1;
            uint16_t h = y2 - y1 + 1;
            bitmap_set_pixels(dst_bmp, x1, y1, w, h, 0, 0, 0, 0);
        }
    }
    if (d_y2 > s_y2) {
        int16_t y1 = s_y2 < 0 ? 0 : s_y2 + 1;
        TRACE_BMP("clear(B) 0 %d %d %d\r\n", y1, d_x2, d_y2); 
        uint16_t w = d_x2 + 1;
        uint16_t h = d_y2 - y1 + 1;
        bitmap_set_pixels(dst_bmp, 0, y1, w, h, 0, 0, 0, 0);
    }
}
