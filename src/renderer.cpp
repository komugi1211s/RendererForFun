#include "maths.h"
#include "renderer.h"

#define USE_LINE_SWEEPING 0
#define _CUR_BUF(buf)     (buf->is_drawing_first ? buf->second_buffer : buf->first_buffer)


void swap_buffer(Drawing_Buffer *buffer) {
    buffer->is_drawing_first = !buffer->is_drawing_first;
}

void clear_buffer(Drawing_Buffer *buffer, Color color) {
    uint32 *buf = _CUR_BUF(buffer);
    uint32 total_size = buffer->window_width * buffer->window_height;

    uint32 _color = color.to_uint32_argb();
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

    uint32 c = color.to_uint32_argb();

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

// NOTE(fuzzy):
// 参考: https://shikihuiku.wordpress.com/2017/05/23/barycentric-coordinates%E3%81%AE%E8%A8%88%E7%AE%97%E3%81%A8perspective-correction-partial-derivative%E3%81%AB%E3%81%A4%E3%81%84%E3%81%A6/

void draw_filled_triangle(Drawing_Buffer *buffer, fVector3 A, fVector3 B, fVector3 C, Color color) {
    // if (USE_LINE_SWEEPING) _line_sweeping(buffer, y_sm, y_md, y_bg, color);
    uint32 *buf = _CUR_BUF(buffer);
    BoundingBoxf3 bd_box = BB_fV3(A, B, C);
    uint32 c = color.to_uint32_argb();
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
                if (gamma1 > 1 || gamma2 > 1 || gamma3 > 1) continue;

                // (1 - x - y)A + xB + yC として定義してあるので
                // 掛け算もそれに合わせて揃えないといけない
                real32 z_value = (A.z * gamma3) + (B.z * gamma1) + (C.z * gamma2);
                int32 position = (y * buffer->window_width) + x;
                if (position > (buffer->window_width * buffer->window_height)) continue;
                if (position < 0) continue;

                if (buffer->z_buffer[position] < z_value) {
                    buffer->z_buffer[position] = z_value;
                    buf[position] = c;
                }
            }
        }
    }
}

// draw_filled_triangleとほとんど同じ
void draw_triangle_with_texture(Drawing_Buffer *buffer, Texture *texture, fVector3 texcoords[3],
                                real32 light_intensity, fVector3 A, fVector3 B, fVector3 C) 
{
    PROFILE_FUNC
    uint32 *buf = _CUR_BUF(buffer);
    BoundingBoxf3 bd_box = BB_fV3(A, B, C);
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

                if (position > (buffer->window_width * buffer->window_height)) continue;
                if (position < 0) continue;
                if (x > buffer->window_width || x < 0) continue;

                real32 t_x = (texcoords[0].x * gamma3) + (texcoords[1].x * gamma1) + (texcoords[2].x * gamma2);
                real32 t_y = (texcoords[0].y * gamma3) + (texcoords[1].y * gamma1) + (texcoords[2].y * gamma2);

                int32 tex_x = t_x * texture->width;
                int32 tex_y = t_y * texture->height;
                int32 tex_p = (int32)(tex_x + (tex_y * texture->width)) * texture->channels;

                uint32 r, g, b;
                r = texture->data[tex_p]     * light_intensity;
                g = texture->data[tex_p + 1] * light_intensity;
                b = texture->data[tex_p + 2] * light_intensity;

                if (buffer->z_buffer[position] < z_value) {
                    buffer->z_buffer[position] = z_value;
                    buf[position] = (uint32)(255 << 24 | r << 16 | g << 8 | b);
                }
            }
        }
    }
}

void draw_triangle_wireframe(Drawing_Buffer *buffer, iVector2 A, iVector2 B, iVector2 C, Color color) {
    draw_line(buffer, A.x, A.y, B.x, B.y, color);
    draw_line(buffer, A.x, A.y, C.x, C.y, color);
    draw_line(buffer, B.x, B.y, C.x, C.y, color);
}

// I think I can do simd???
void project_viewport(fVector3 world_space[3],
                      int32 window_width,
                      int32 window_height)
{
    world_space[0].x = ((1.0 + world_space[0].x) * (window_width - 1)  / 2);
    world_space[0].y = ((1.0 + world_space[0].y) * (window_height - 1)  / 2);

    world_space[1].x = ((1.0 + world_space[1].x) * (window_width - 1)  / 2);
    world_space[1].y = ((1.0 + world_space[1].y) * (window_height - 1)  / 2);

    world_space[2].x = ((1.0 + world_space[2].x) * (window_width - 1)  / 2);
    world_space[2].y = ((1.0 + world_space[2].y) * (window_height - 1)  / 2);
}

void draw_filled_rectangle(Drawing_Buffer *buffer, fVector3 position, fVector3 size, Color color) {
    uint32 *buf = _CUR_BUF(buffer);

    int32 x0, y0, x1, y1;
    x0 = (int32)floor(position.x);
    x1 = (int32)ceil(position.x + size.x);

    y0 = (int32)floor(position.y);
    y1 = (int32)ceil(position.y + size.y);

    if (x0 < 0 || y0 < 0) return;
    if (x1 > buffer->window_width || y1 > buffer->window_height) return;

    for (int32 y = y0; y < y1; ++y) {
        for (int32 x = x0; x < x1; ++x) {
            int32 position = (y * buffer->window_width) + x;
            uint32 current_color = buf[position];
            Color c = Color::from_uint32_argb(current_color);
            c = color.blend(c);
            buf[position] = c.to_uint32_argb();
        }
    }
}

void project_perspective(Camera *cam, fVector3 *target);
void project_view(Camera *cam, fVector3 *target);
void draw_model(Drawing_Buffer *buffer, Model *model, Camera *camera, Texture *texture) {
    PROFILE_FUNC
    fVector3 light_pos = fVec3(0.0, 0.0, -10.0);
    fVector3 AB, AC;
    fVector3 surface_normal, light_normal;
    real32 color_intensity;

    fVector3 vertices[3]  = {0};
    fVector3 texcoords[3] = {0};

    real32 aspect_ratio = 16.0 / 9.0;
    for (size_t i = 0; i < model->num_indexes; ++i) {
        bool32 vert_loaded = load_model_vertices(model, i, vertices);
        bool32 tex_loaded  = load_model_texcoords(model, i, texcoords);

        if (vert_loaded && tex_loaded) {
            for (int i = 0; i < 3; ++i) {
                project_view(camera, &vertices[i]);
                project_perspective(camera, &vertices[i]);
            }

            AB = fsub_fv3(vertices[1], vertices[0]);
            AC = fsub_fv3(vertices[2], vertices[0]);

            surface_normal  = fnormalize_fv3(fcross_fv3_simd(AC, AB));
            light_normal    = fnormalize_fv3(light_pos);
            color_intensity = fdot_fv3(light_normal, surface_normal);

            if (color_intensity < 0) continue;
            project_viewport(vertices, buffer->window_width,
                                       buffer->window_height);

            draw_triangle_with_texture(buffer, texture, texcoords, color_intensity,
                                       vertices[0], vertices[1], vertices[2]);
        }
    }
}

void draw_model_wireframe(Drawing_Buffer *buffer, Model *model, Color color) {
    fVector3 vertex[3];

    for (size_t i = 0; i < model->num_indexes; ++i) {
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
    PROFILE_FUNC
    int32 x = start_x;
    int32 y = start_y + (font->base_line + font->line_gap);
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
            DEBUGCharacterBitmap bitmap;
            bitmap.allocated = stbtt_GetCodepointBitmap(&font->font_info, font->char_scale, font->char_scale,
                                                         character, &bitmap.width, &bitmap.height, &bitmap.x_off, &bitmap.y_off);
            if (bitmap.allocated) {
                bitmap_array[character] = bitmap;
            } else {
                bitmap_array[character].allocated = NULL;
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

int32 get_text_width(FontData *font, char *text) {
    int32 x = 0;
    for(char *t = text; *t; ++t) {
        char character = *t;
        int32 advance, lsb;
        stbtt_GetCodepointHMetrics(&font->font_info, character, &advance, &lsb);
        x += (advance + lsb) * font->char_scale;
    }
    return x;
}

void project_perspective(Camera *cam, fVector3 *target) {
    /* apply projection for left-handed y-up perspective. */
    fVector4 copied;
    copied.x = target->x;
    copied.y = target->y;
    copied.z = target->z;
    copied.w = 1.0;

    real32 tan_fov = tanf(cam->fov / 2.0);
    fMat4x4 proj = {0};

    proj.row[0].col[0] = 1.0f / (cam->aspect_ratio * tan_fov);
    proj.row[1].col[1] = 1.0f / tan_fov;
    proj.row[2].col[2] = (cam->z_near + cam->z_far) / (cam->z_far - cam->z_near);
    proj.row[2].col[3] = -(2.0 * cam->z_far * cam->z_near) / (cam->z_far - cam->z_near);
    proj.row[3].col[2] = -1.0f;

    copied = fmul_fmat4x4_fv4(proj, copied);
    target->x = copied.x;
    target->y = copied.y;
    target->z = copied.z;
}

void project_view(Camera *cam, fVector3 *target) {
    /*
     * apply lookat view based on camera.
     * 注意: 毎回方向ベクターを計算するのは無駄かも知れない。
     * */

    fVector4 copied;
    copied.x = target->x;
    copied.y = target->y;
    copied.z = target->z;
    copied.w = 1.0;

    fVector3 direction, right, up;
    fVector3 looking_at = fadd_fv3(cam->position, cam->target);
    direction = fnormalize_fv3(fsub_fv3(looking_at, cam->position));
    right     = fnormalize_fv3(fcross_fv3(cam->up, direction));
    up        = fnormalize_fv3(fcross_fv3(direction, right));

    fMat4x4 lookat = {0};
    lookat.row[0].col[0] = right.x;
    lookat.row[0].col[1] = right.y;
    lookat.row[0].col[2] = right.z;

    lookat.row[1].col[0] = up.x;
    lookat.row[1].col[1] = up.y;
    lookat.row[1].col[2] = up.z;

    lookat.row[2].col[0] = direction.x;
    lookat.row[2].col[1] = direction.y;
    lookat.row[2].col[2] = direction.z;

    lookat.row[0].col[3] = -fdot_fv3(right,     cam->position);
    lookat.row[1].col[3] = -fdot_fv3(up,        cam->position);
    lookat.row[2].col[3] = -fdot_fv3(direction, cam->position);
    lookat.row[3].col[3] = 1.0;

    copied = fmul_fmat4x4_fv4(lookat, copied);
    target->x = copied.x;
    target->y = copied.y;
    target->z = copied.z;
}
