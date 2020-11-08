#include "maths.h"
#include "renderer.h"

#define USE_LINE_SWEEPING 0
#define _CUR_BUF(buf)     (buf->is_drawing_first ? buf->first_buffer : buf->second_buffer)

uint32 color_to_uint32(Color *color) {
    uint32 r, g, b, a;

    a = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(color->a, 1.0)) * 255.0);
    r = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(color->r, 1.0)) * 255.0);
    g = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(color->g, 1.0)) * 255.0);
    b = (uint8)floor(MATHS_MAX(0.0, MATHS_MIN(color->b, 1.0)) * 255.0);

    return (a << 24 | r << 16 | g << 8 | b);
}

void swap_buffer(Drawing_Buffer *buffer) {
    buffer->is_drawing_first = !buffer->is_drawing_first;
}

void clear_buffer(Drawing_Buffer *buffer, Color color) {
    uint32 *buf = _CUR_BUF(buffer);
    uint32 total_size = buffer->window_width * buffer->window_height;
    
    for (uint32 t = 0; t <= total_size; ++t) {
        buf[t] = color_to_uint32(&color);
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

void draw_bounding_box(Drawing_Buffer *buffer, BoundingBox box, Color color) {

    // from top left, goes right / bottom side
    draw_line(buffer, box.min_v.x, box.min_v.y, box.min_v.x, box.max_v.y, color);
    draw_line(buffer, box.min_v.x, box.min_v.y, box.max_v.x, box.min_v.y, color);

    // from bottom left, goes right side
    draw_line(buffer, box.min_v.x, box.max_v.y, box.max_v.x, box.max_v.y, color);

    // from top right,   goes bottom side
    draw_line(buffer, box.max_v.x, box.min_v.y, box.max_v.x, box.max_v.y, color);

}

void _barycentric_containment_detection(Drawing_Buffer *buffer, iVector2 A, 
                                        iVector2       B,       iVector2 C,
                                        Color color)
{
    uint32 *buf = _CUR_BUF(buffer);
    BoundingBox bd_box = BB_iV2_Triangle(A, B, C);

    iVector2 AB = iVec2(B.x - A.x, B.y - A.y);
    iVector2 AC = iVec2(C.x - A.x, C.y - A.y);

    uint32 c = color_to_uint32(&color);

    for(int32 y = bd_box.min_v.y; y < bd_box.max_v.y; ++y) {
        // [ABy, ACy, PAy]を作る
        fVector3 cross_y = fVec3((real32)AB.y, (real32)AC.y, (real32)(A.y - y));

        for(int32 x = bd_box.min_v.x; x < bd_box.max_v.x; ++x) {
            // [ABx, ACx, PAx]を作る
            fVector3 cross_x = fVec3((real32)AB.x, (real32)AC.x, (real32)(A.x - x));

            // [AB, AC, PA] x, y と垂直なベクター[u, v, 1]を得る.
            // この際 uv.z は 本来3Dのベクターが得るはずの面積を指し示すので、
            // これでx, y, zを割って正規化する
            fVector3 uv = fcross_fv3(cross_x, cross_y);

            // ここに正の整数が入っていない場合、
            // 線が重なっている もしくは 肝心の方向が逆になっている場合がある( AxB ~= BxA )
            // 完全に拒絶
            if (abs(uv.z) < 1) {
                goto skipped_the_entire_loop;
            }

            // 正規化
            uv.x /= uv.z;
            uv.y /= uv.z;

            if ((1.f - uv.x - uv.y) < 0 || uv.x < 0 || uv.y < 0) continue;
            buf[(y * buffer->window_width) + x] = c;
        }
    }

skipped_the_entire_loop:
    draw_line(buffer, A.x, A.y, B.x, B.y, color);
    draw_line(buffer, A.x, A.y, C.x, C.y, color);
    draw_line(buffer, B.x, B.y, C.x, C.y, color);
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
        real32 total_dt    = (y - y_sm.y) / total_distance;
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

void draw_triangle(Drawing_Buffer *buffer, iVector2 y_sm, iVector2 y_md, iVector2 y_bg, Color color) {
    if (USE_LINE_SWEEPING) _line_sweeping(buffer, y_sm, y_md, y_bg, color);
    else                   _barycentric_containment_detection(buffer, y_sm, y_md, y_bg, color);
}

void draw_triangle_wireframe(Drawing_Buffer *buffer, iVector2 A, iVector2 B, iVector2 C, Color color) {
    draw_line(buffer, A.x, A.y, B.x, B.y, color);
    draw_line(buffer, A.x, A.y, C.x, C.y, color);
    draw_line(buffer, B.x, B.y, C.x, C.y, color);
}

void draw_model(Drawing_Buffer *buffer, Model *model, Color color) {
    ModelVertex vertex;
    real32 max_value_for_teapot = 3.0;

    fVector3 light_pos = fVec3(0.0, 0.0, -10.0);
    for (size_t i = 0; i < model->num_indexes; ++i) {
        if (load_model_vertices(model, i, vertex)) {
            iVector2 one, two, three;
            fVector3 AB, AC;
            AB = fsub_fv3(vertex[1], vertex[0]);
            AC = fsub_fv3(vertex[2], vertex[0]);

            fVector3 surface_normal = fnormalize_fv3(fcross_fv3(AC, AB));
            fVector3 light_normal   = fnormalize_fv3(light_pos);
            real32 color_intensity = fdot_fv3(light_normal, surface_normal);

            if (color_intensity < 0) continue;

            Color new_color;
            new_color.r = color.r * color_intensity;
            new_color.g = color.g * color_intensity;
            new_color.b = color.b * color_intensity;
            new_color.a = color.a * color_intensity;

            one.x   = (int32)((1.0 + vertex[0].x) * (buffer->window_width - 1)  / 2);
            one.y   = (int32)((1.0 + vertex[0].y) * (buffer->window_height - 1) / 2);

            two.x   = (int32)((1.0 + vertex[1].x) * (buffer->window_width - 1)  / 2);
            two.y   = (int32)((1.0 + vertex[1].y) * (buffer->window_height - 1) / 2);

            three.x = (int32)((1.0 + vertex[2].x) * (buffer->window_width - 1)  / 2);
            three.y = (int32)((1.0 + vertex[2].y) * (buffer->window_height - 1) / 2);

            draw_triangle(buffer, one, two, three, new_color);
        }
    }
}

void draw_model_wireframe(Drawing_Buffer *buffer, Model *model, Color color) {
    ModelVertex vertex;
    real32 max_value_for_teapot = 3.0;

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
