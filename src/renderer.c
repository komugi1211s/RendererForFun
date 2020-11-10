#include "maths.h"
#include "renderer.h"

#define USE_LINE_SWEEPING 0
#define _CUR_BUF(buf)     (buf->is_drawing_first ? buf->second_buffer : buf->first_buffer)

// TODO(fuzzy): BGRA系統だと動かない
uint32 color_to_uint32(Color *color) {
    uint8 r, g, b, a;

    a = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(color->a, 1.0)) * 255.0);
    r = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(color->r, 1.0)) * 255.0);
    g = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(color->g, 1.0)) * 255.0);
    b = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(color->b, 1.0)) * 255.0);

    uint32 result = ((a << 24) | (r << 16) | (g << 8) | b);
    return result;
}

void swap_buffer(Drawing_Buffer *buffer) {
    buffer->is_drawing_first = !buffer->is_drawing_first;
}

void clear_buffer(Drawing_Buffer *buffer, Color color) {
    uint32 *buf = _CUR_BUF(buffer);
    uint32 total_size = buffer->window_width * buffer->window_height;

    uint32 _color = color_to_uint32(&color);
    real32 z_buffer_min = -1.175494351e-38f;
    for (int32 i = 0; i < total_size; ++i) {
        buf[i] = _color;
    }
    for (int32 i = 0; i < total_size; ++i) {
        buffer->z_buffer[i] = z_buffer_min;
    }
}

void draw_line(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, Color color) {
    ASSERT(0 <= x0 && 0 <= x1 && 0 <= y0 && 0 <= y1);
    ASSERT(x1 < buffer->window_width);
    ASSERT(y1 < buffer->window_height);
    uint32 *buf = _CUR_BUF(buffer);

    int32 x_start, x_end, y_start, y_end;
    bool32 transpose = abs(x1 - x0) < abs(y1 - y0);
    x_start = transpose ? y0 : x0;
    y_start = transpose ? x0 : y0;
    x_end   = transpose ? y1 : x1;
    y_end   = transpose ? x1 : y1;

    if (x_start > x_end) {
        SWAPVAR(int32, x_start, x_end);
        SWAPVAR(int32, y_start, y_end);
    }

    real32 delta = (real32)(x_end - x_start);
    ASSERT(delta >= 0);
    bool32 parallel_to_axis = y_end == y_start;

    uint32 c = color_to_uint32(&color);

    // NOTE(fuzzy): Testing optimization.
    if (parallel_to_axis) {
        for (int32 t = x_start; t <= x_end; ++t) {
            int32 x = t;
            int32 y = y_start;
            if (transpose) {
                x = y;
                y = t;
            }
            buf[(y * buffer->window_width) + x] = c;
        }
    } else {
        for (int32 t = x_start; t <= x_end; ++t) {
            real32 dt = (t - x_start) / delta;
            int32 x = t;
            int32 y = ((1.0 - dt) * y_start + dt * y_end);
            if (transpose) {
                x = y;
                y = t;
            }
            buf[(y * buffer->window_width) + x] = c;
        }
    }
}

internal inline void
draw_bounding_box(Drawing_Buffer *buffer, BoundingBoxi2 box, Color color) {

    // from top left, goes right / bottom side
    draw_line(buffer, box.min_v.x, box.min_v.y, box.min_v.x, box.max_v.y, color);
    draw_line(buffer, box.min_v.x, box.min_v.y, box.max_v.x, box.min_v.y, color);

    // from bottom left, goes right side
    draw_line(buffer, box.min_v.x, box.max_v.y, box.max_v.x, box.max_v.y, color);

    // from top right,   goes bottom side
    draw_line(buffer, box.max_v.x, box.min_v.y, box.max_v.x, box.max_v.y, color);

}

// NOTE(fuzzy):
// 参考: https://shikihuiku.wordpress.com/2017/05/23/barycentric-coordinates%E3%81%AE%E8%A8%88%E7%AE%97%E3%81%A8perspective-correction-partial-derivative%E3%81%AB%E3%81%A4%E3%81%84%E3%81%A6/
internal void
barycentric_drawing(Drawing_Buffer *buffer, fVector3 A, fVector3 B, fVector3 C, Color color)
{
    PROFILE_FUNC_START;
    uint32 *buf = _CUR_BUF(buffer);
    BoundingBoxf3 bd_box = BB_fV3(A, B, C);
    uint32 c = color_to_uint32(&color);
    fVector3 AB, AC, PA;

    // 最適化用に iVec2 は使わない（使うと鬼のように遅くなる）
    AB.x = B.x - A.x;
    AB.y = B.y - A.y;
    AC.x = C.x - A.x;
    AC.y = C.y - A.y;

    real32 det_T = AB.x * AC.y - AC.x * AB.y;
    bool32 triangle_is_degenerate = fabsf(det_T) < 1;

    if (!triangle_is_degenerate) {
        int32 y_min = (int32)floor(bd_box.min_v.y);
        int32 y_max = (int32)ceil(bd_box.max_v.y);
        int32 x_min = (int32)floor(bd_box.min_v.x);
        int32 x_max = (int32)ceil(bd_box.max_v.x);

        for(int32 y = y_min; y < y_max; ++y) {
            PA.y = y - A.y;
            for(int32 x = x_min; x < x_max; ++x) {
                PA.x = x - A.x;
                real32 gamma1, gamma2, gamma3;

                gamma1 = (PA.x * AC.y - PA.y * AC.x) / det_T;
                gamma2 = (AB.x * PA.y - AB.y * PA.x) / det_T;
                gamma3 = 1.f - gamma1 - gamma2;

                // (1 - x - y)A + xB + yC = P' なので x+y+(1-x-y) = 1 とならなければならず
                // どれか1つの点が負数の場合三角形の外に点があることになる
                if (gamma1 < 0 || gamma2 < 0 || gamma3 < 0) continue;

                // (1 - x - y)A + xB + yC として定義してあるので
                // 掛け算もそれに合わせて揃えないといけない
                real32 z_value = (A.z * gamma3) + (B.z * gamma1) + (C.z * gamma2);
                int32 position = (y * buffer->window_width) + x;
                if (buffer->z_buffer[position] < z_value) {
                    buffer->z_buffer[position] = z_value;
                    buf[position] = c;
                }
            }
        }
    }
    PROFILE_FUNC_END;
}

void _line_sweeping(Drawing_Buffer *buffer, iVector2 y_sm, iVector2 y_md, iVector2 y_bg, Color color) {
    if (y_sm.y > y_md.y) { SWAPVAR(iVector2, y_sm, y_md); }
    if (y_sm.y > y_bg.y) { SWAPVAR(iVector2, y_sm, y_bg); }
    if (y_md.y > y_bg.y) { SWAPVAR(iVector2, y_md, y_bg); }

    draw_line(buffer, y_sm.x, y_sm.y, y_md.x, y_md.y, color);
    draw_line(buffer, y_sm.x, y_sm.y, y_bg.x, y_bg.y, color);
    draw_line(buffer, y_md.x, y_md.y, y_bg.x, y_bg.y, color);

    real32 top_half_distance    = (y_md.y - y_sm.y);
    real32 total_distance       = (y_bg.y - y_sm.y);
    real32 bottom_half_distance = (y_bg.y - y_md.y);

    int32 x0, x1;
    int32 y = y_sm.y;

    for (; y < y_md.y; ++y) {
        // Draw top half.
        real32 total_dt    = (y - y_sm.y)  / total_distance;
        real32 top_half_dt = (y - y_sm.y)  / top_half_distance;
        x0 = (y_sm.x * (1.0 - top_half_dt)) + (top_half_dt * y_md.x);
        x1 = (y_sm.x * (1.0 - total_dt)) + (total_dt * y_bg.x);

        draw_line(buffer, x0, y, x1, y, color);
    }

    for (; y < y_bg.y; ++y) {
        // Draw bottom Half.
        real32 total_dt       = (y - y_sm.y) / total_distance;
        real32 bottom_half_dt = (y - y_md.y) / bottom_half_distance;
        x0 = (y_md.x *  (1.0 - bottom_half_dt)) + (bottom_half_dt * y_bg.x);
        x1 = (y_sm.x  * (1.0 - total_dt)) + (total_dt * y_bg.x);
        draw_line(buffer, x0, y, x1, y, color);
    }
}

void draw_triangle(Drawing_Buffer *buffer, fVector3 A, fVector3 B, fVector3 C, Color color) {
    // if (USE_LINE_SWEEPING) _line_sweeping(buffer, y_sm, y_md, y_bg, color);

    barycentric_drawing(buffer, A, B, C, color);
}

void draw_triangle_wireframe(Drawing_Buffer *buffer, iVector2 A, iVector2 B, iVector2 C, Color color) {
    draw_line(buffer, A.x, A.y, B.x, B.y, color);
    draw_line(buffer, A.x, A.y, C.x, C.y, color);
    draw_line(buffer, B.x, B.y, C.x, C.y, color);
}

// I think I can do simd???
void turn_vertex_into_screen_space(ModelVertex world_space,
                                   int32 window_width, int32 window_height)
{
    world_space[0].x = ((1.0 + world_space[0].x) * (window_width - 1)  / 2);
    world_space[0].y = ((1.0 + world_space[0].y) * (window_height - 1)  / 2);

    world_space[1].x = ((1.0 + world_space[1].x) * (window_width - 1)  / 2);
    world_space[1].y = ((1.0 + world_space[1].y) * (window_height - 1)  / 2);

    world_space[2].x = ((1.0 + world_space[2].x) * (window_width - 1)  / 2);
    world_space[2].y = ((1.0 + world_space[2].y) * (window_height - 1)  / 2);
}

void draw_model(Drawing_Buffer *buffer, Model *model, Color color) {
    PROFILE_FUNC_START;
    ModelVertex vertex; // typedef to fVector3[3]
    fVector3 light_pos = fVec3(0.0, 0.0, -10.0);
    fVector3 AB, AC;
    fVector3 surface_normal, light_normal;
    real32 color_intensity;

    for (size_t i = 0; i < model->num_vert_idx; ++i) {
        if (load_model_vertices(model, i, vertex)) {
            AB = fsub_fv3(vertex[1], vertex[0]);
            AC = fsub_fv3(vertex[2], vertex[0]);

            PROFILE("calculate light direction") {
                surface_normal = fnormalize_fv3(fcross_fv3_simd(AC, AB));
                light_normal   = fnormalize_fv3(light_pos);
                color_intensity = fdot_fv3(light_normal, surface_normal);
            }

            if (color_intensity < 0) continue;

            Color new_color;
            new_color.r = color.r * color_intensity;
            new_color.g = color.g * color_intensity;
            new_color.b = color.b * color_intensity;
            new_color.a = color.a * color_intensity;

            turn_vertex_into_screen_space(vertex, buffer->window_width, buffer->window_height);
            draw_triangle(buffer, vertex[0], vertex[1], vertex[2], new_color);
        }
    }
    PROFILE_FUNC_END;
}

void draw_model_wireframe(Drawing_Buffer *buffer, Model *model, Color color) {
    ModelVertex vertex;

    for (size_t i = 0; i < model->num_vert_idx; ++i) {
        if (load_model_vertices(model, i, vertex)) {
            iVector2 one, two, three;

            one.x   = (int32)((1.0 + vertex[0].x) * (buffer->window_width  - 1) / 2);
            one.y   = (int32)((1.0 + vertex[0].y) * (buffer->window_height - 1) / 2);

            two.x   = (int32)((1.0 + vertex[1].x) * (buffer->window_width  - 1) / 2);
            two.y   = (int32)((1.0 + vertex[1].y) * (buffer->window_height - 1) / 2);

            three.x = (int32)((1.0 + vertex[2].x) * (buffer->window_width  - 1) / 2);
            three.y = (int32)((1.0 + vertex[2].y) * (buffer->window_height - 1) / 2);

            if (one.x   >= buffer->window_width) TRACE("One.x overflow. %d", one.x);
            if (one.y   >= buffer->window_height) TRACE("One.y overflow. %d", one.y);
            if (two.x   >= buffer->window_width) TRACE("Two.x overflow. %d", two.x);
            if (two.y   >= buffer->window_height) TRACE("Two.y overflow. %d", two.y);
            if (three.x >= buffer->window_width) TRACE("Three.x overflow. %d", three.x);
            if (three.y >= buffer->window_height) TRACE("Three.y overflow. %d", three.y);

            draw_triangle_wireframe(buffer, one, two, three, color);
        }
    }
}


typedef struct DEBUGCharacterBitmap {
    uint8 *allocated;
    int32 width, height, x_off, y_off;
} DEBUGCharacterBitmap;

#define BITMAP_ARRAY_SIZE 1024
global_variable DEBUGCharacterBitmap bitmap_array[BITMAP_ARRAY_SIZE] = {0}; // THIS IS TOO STUPID

void draw_text(Drawing_Buffer *buffer, FontData *font, int32 start_x, int32 start_y, char *text) {

    int32 x = start_x;
    int32 y = start_y;
    uint32 *buf = _CUR_BUF(buffer);

    for (char *t = text; *t; ++t) {
        if (*t == '\n') {
            y += (font->base_line + font->line_gap);
            continue;
        }
        char character = *t;
        int32 advance, lsb, x0, x1, y0, y1;
        stbtt_GetCodepointHMetrics(&font->font_info, character, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font->font_info, character, font->char_scale, font->char_scale,
                                    &x0, &y0, &x1, &y1);

        DEBUGCharacterBitmap *bitmap;
        if (!bitmap_array[character].allocated) {
            printf("Creating bitmap for :%c\n", character);

            DEBUGCharacterBitmap bitmap;
            bitmap.allocated = stbtt_GetCodepointBitmap(&font->font_info, font->char_scale, font->char_scale,
                                                         character, &bitmap.width, &bitmap.height, &bitmap.x_off, &bitmap.y_off);
            if (bitmap.allocated) {
                bitmap_array[character] = bitmap;
            } else {
                bitmap_array[character].allocated = (uint8 *)0xFFFFFFFFFFFFFFF;
                bitmap_array[character].width = -1;
                bitmap_array[character].height = -1;
            }
        }
        bitmap = &bitmap_array[character];

        x += (advance + lsb) * font->char_scale;
        int32 position = (x + (y + y0) * buffer->window_width);

        if (bitmap->width < 0) continue;
        for (int i = 0; i < bitmap->height; ++i) {
            for (int j = 0; j < bitmap->width; ++j) {
                if (bitmap->allocated[j + (i * bitmap->width)] > 0) {
                    uint8 color = bitmap->allocated[j + (i * bitmap->width)];
                    buf[position + (j + (i * buffer->window_width))] = (color << 16 | color << 8 | color);
                }
            }
        }
    }

}



