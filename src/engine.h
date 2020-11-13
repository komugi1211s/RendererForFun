#ifndef _K_ENGINE_H
#define _K_ENGINE_H

typedef struct MouseInput {
    real32 x, y;
    bool32 left, right;
} MouseInput;

typedef struct Input {
    bool32 forward, backward, left, right;
    MouseInput mouse;
    MouseInput prev_mouse;
    bool32 debug_menu_key;
} Input;

typedef struct FontData {
    stbtt_fontinfo font_info;
    int32          ascent;
    int32          descent;
    int32          line_gap;
    int32          base_line;
    real32         char_scale;
} FontData;

typedef struct Color {
    real32 r, g, b, a;

    uint32 to_uint32_argb() {
        uint8 u8r, u8g, u8b, u8a;

        u8a = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(a, 1.0)) * 255.0);
        u8r = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(r, 1.0)) * 255.0);
        u8g = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(g, 1.0)) * 255.0);
        u8b = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(b, 1.0)) * 255.0);

        uint32 result = ((u8a << 24) | (u8r << 16) | (u8g << 8) | u8b);
        return result;
    }

    static Color from_uint32_argb(uint32 from) {
        real32 ra, rg, rb, rr;

        ra = (real32)((from & 0xFF000000) >> 24) / 255.0;
        rr = (real32)((from & 0x00FF0000) >> 16) / 255.0;
        rg = (real32)((from & 0x0000FF00) >>  8) / 255.0;
        rb = (real32)((from & 0x000000FF)      ) / 255.0;

        Color result;
        result.a = ra;
        result.r = rr;
        result.g = rg;
        result.b = rb;
        return result;
    }

    Color blend(Color other) {
        /*
         * Typical Alpha multiply Blend
         * https://ja.wikipedia.org/wiki/%E3%82%A2%E3%83%AB%E3%83%95%E3%82%A1%E3%83%96%E3%83%AC%E3%83%B3%E3%83%89
         * */

        Color result = {0};
        result.a = a + other.a * (1.0 - a);
        result.r = r + other.r * (1.0 - a);
        result.g = g + other.g * (1.0 - a);
        result.b = b + other.b * (1.0 - a);

        return result;
    }
} Color;

Color rgba(real32 r, real32 g, real32 b, real32 a) {
    Color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = a;

    return result;
}

Color rgb_opaque(real32 r, real32 g, real32 b) {
    Color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = 1.0;

    return result;
}
#endif
