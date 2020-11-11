#ifndef _K_RENDERER_H
#define _K_RENDERER_H

#include "general.h"
#include "maths.h"
#include "model.h"

typedef struct Drawing_Buffer {
    int32 window_width;
    int32 window_height;

    int32 depth;
    bool32 is_drawing_first;

    uint32 *first_buffer;
    uint32 *second_buffer;
    real32 *z_buffer;
} Drawing_Buffer;


typedef struct Camera {
    fVector3 position;
    fVector3 target;
    fVector3 up;

    real32 fov;
    real32 aspect_ratio;
    real32 z_near;
    real32 z_far;

    real32 pitch, yaw;
} Camera;

// TODO(fuzzy):
// RGB/BGR format.

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
         * https://ja.wikipedia.org/wiki/%E3%82%A2%E3%83%AB%E3%83%95%E3%82%A1%E3%83%81%E3%83%A3%E3%83%B3%E3%83%8D%E3%83%AB
         * */

        Color result = {0};
        result.r = r * other.r;
        result.g = g * other.g;
        result.b = b * other.b;
        result.a = a * other.a;

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

void swap_buffer(Drawing_Buffer *buffer);
void clear_buffer(Drawing_Buffer *buffer, Color color);
void draw_line(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, Color color);
void draw_triangle(Drawing_Buffer *buffer, fVector3 A, fVector3 B, fVector3 C, Color color);
void draw_model(Drawing_Buffer *buffer, Model *model, Camera *camera, Texture *texture);
void project_perspective(Camera *cam, fVector3 *target);
void project_view(Camera *cam, fVector3 *target);

#ifdef STB_TRUETYPE_IMPLEMENTATION

typedef struct FontData {
    stbtt_fontinfo font_info;
    int32          ascent;
    int32          descent;
    int32          line_gap;
    int32          base_line;
    real32         char_scale;
} FontData;

void draw_text(Drawing_Buffer *buffer, FontData *font_data, int32 x, int32 y, char *Text);

#endif // STB_TRUETYPE_IMPLEMENTATION
#endif // _K_RENDERER_H
