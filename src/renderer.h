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
    real32 *z_buffer;

} Drawing_Buffer;


typedef struct Camera {
    fVector3 position;
    fVector3 target;
    fVector3 up;
} Camera;

// TODO(fuzzy):
// RGB/BGR format.

typedef struct Color {
    real32 r, g, b, a;
} Color;

Color rgb_opaque(real32 r, real32 g, real32 b) {
    Color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = 0.0;

    return result;
}

void swap_buffer(Drawing_Buffer *buffer);
void clear_buffer(Drawing_Buffer *buffer, Color color);

uint32 color_to_uint32(Color *color);

void draw_line(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, Color color);
void draw_triangle(Drawing_Buffer *buffer, fVector3 A, fVector3 B, fVector3 C, Color color);

void draw_model(Drawing_Buffer *buffer, Model *model, Texture *texture);
void project_perspective(Camera *cam, fVector3 *target, real32 fov, real32 aspect_ratio, real32 z_near, real32 z_far);

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
