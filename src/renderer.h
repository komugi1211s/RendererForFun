#ifndef _K_RENDERER_H
#define _K_RENDERER_H

#include "general.h"
#include "maths.h"
#include "model.h"
#include "engine.h"

typedef struct ScreenBuffer {
    int32 window_width;
    int32 window_height;

    int32 depth;
    bool32 is_drawing_first;

    uint32 *visible_buffer;
    uint32 *back_buffer;
    real32 *z_buffer;
} ScreenBuffer;


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


global_variable const uint32 CLEAR_COLOR_BUFFER = 1 << 1;
global_variable const uint32 CLEAR_Z_BUFFER     = 1 << 2;

// TODO(fuzzy): Replace with glyphs
typedef struct DEBUGCharacterBitmap {
    uint8 *allocated;
    int32 width, height, x_off, y_off;
} DEBUGCharacterBitmap;

#define BITMAP_ARRAY_SIZE 1024
global_variable DEBUGCharacterBitmap bitmap_array[BITMAP_ARRAY_SIZE] = {0}; // THIS IS TOO STUPID


// TODO(fuzzy):
// RGB/BGR format.

//
/* TODO(fuzzy):
現在のデザインは間違いなくいつか自分の足を引っ張ると思うので
それなりにまともなデザインを考えておくこと

 - 重心計算を露出させる（三角形を書くエリアで行ってはならない）
     - 少し計算が遅くなるかも知れないけど、遅くなったら他の時に考えよう

*/

void swap_buffer(ScreenBuffer *buffer);
void clear_buffer(ScreenBuffer *buffer, uint32 clear_mode, real32 z_max);
void draw_line(ScreenBuffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, Color color);

void draw_wire_triangle(ScreenBuffer *buffer, fVector3 triangle[3], Color color);
void draw_filled_triangle(ScreenBuffer *buffer, fVector3 triangle[3], fVector3 light_intensity, Color color);
void draw_textured_triangle(ScreenBuffer *buffer, fVector3 triangle[3], fVector3 texcoords[3], fVector3 light_intensity, Texture *texture);

void draw_wire_model(ScreenBuffer *buffer, Model *model, Camera *camera, Property *property, Color color);
void draw_filled_model(ScreenBuffer *buffer, Model *model, Camera *camera, Property *property, Color color);
void draw_textured_model(ScreenBuffer *buffer, Model *model, Camera *camera, Property *property, Texture *texture);

void draw_wire_rectangle(ScreenBuffer *buffer,   real32 x0, real32 y0, real32 x1, real32 y1, Color color);
void draw_filled_rectangle(ScreenBuffer *buffer, real32 x0, real32 y0, real32 x1, real32 y1, Color color);
void DEBUG_render_z_buffer(ScreenBuffer *buffer, real32 z_max);
void draw_gizmo_to_origin(ScreenBuffer *buffer, Camera *camera);

void draw_text(ScreenBuffer *buffer, FontData *font_data, int32 x, int32 y, char *Text);

#endif // _K_RENDERER_H
