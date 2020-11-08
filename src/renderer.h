#ifndef _K_RENDERER_H
#define _K_RENDERER_H

#include "maths.h"
#include "model.h"

typedef struct Drawing_Buffer {
    int32 window_width;
    int32 window_height;

    int32 depth;
    bool32 is_drawing_first;

    uint32 *first_buffer;
    uint32 *second_buffer;

    int32 *z_buffer;
} Drawing_Buffer;

typedef struct Color {
    real32 r, g, b, a;
} Color;

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

uint32 color_to_uint32(Color *color);

void draw_line(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, Color color);
void draw_bounding_box(Drawing_Buffer *buffer, BoundingBox box, Color color);
void draw_triangle(Drawing_Buffer *buffer, iVector2 y_sm, iVector2 y_md, iVector2 y_bg, Color color);

void draw_model(Drawing_Buffer *buffer, Model *model, Color color);

#endif
