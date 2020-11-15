#ifndef _K_RENDERER_H
#define _K_RENDERER_H

#include "general.h"
#include "maths.h"
#include "model.h"
#include "engine.h"

global_variable fMat4x4 modelview;
global_variable fMat4x4 projection;

global_variable fMat4x4 mvp;
global_variable bool32  mvp_precomputed;

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


global_variable const uint32 CLEAR_COLOR_BUFFER = 1 << 1;
global_variable const uint32 CLEAR_Z_BUFFER     = 1 << 2;


// TODO(fuzzy):
// RGB/BGR format.

void swap_buffer(Drawing_Buffer *buffer);
void clear_buffer(Drawing_Buffer *buffer, uint32 clear_mode);
void draw_line(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, Color color);

void draw_wire_triangle(Drawing_Buffer *buffer, fVector3 A, fVector3 B, fVector3 C, Color color);
void draw_filled_triangle(Drawing_Buffer *buffer, fVector3 A, fVector3 B, fVector3 C, Color color);
void draw_textured_triangle(Drawing_Buffer *buffer, fVector3 A, fVector3 B, fVector3 C, fVector3 texcoords[3], real32 light_intensity, Texture *texture);

void draw_wire_model(Drawing_Buffer *buffer, Model *model, Camera *camera, Color color);
void draw_filled_model(Drawing_Buffer *buffer, Model *model, Camera *camera, Color color);
void draw_textured_model(Drawing_Buffer *buffer, Model *model, Camera *camera, Texture *texture);

void draw_wire_rectangle(Drawing_Buffer *buffer, real32 x0, real32 y0, real32 x1, real32 y1, Color color);
void draw_filled_rectangle(Drawing_Buffer *buffer, real32 x0, real32 y0, real32 x1, real32 y1, Color color);
void DEBUG_render_z_buffer(Drawing_Buffer *buffer);

fMat4x4 create_mvp_matrix(Camera *cam);

void draw_text(Drawing_Buffer *buffer, FontData *font_data, int32 x, int32 y, char *Text);

#endif // _K_RENDERER_H
