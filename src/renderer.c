
#define USE_LINE_SWEEPING 1
#define _CUR_BUF(buf)     buf->is_drawing_first ? buf->first_buffer : buf->second_buffer

void swap_buffer(Drawing_Buffer *buffer) {
    buffer->is_drawing_first = !buffer->is_drawing_first;
}

void draw_line(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1) {
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

    for (int32 t = x_start; t <= x_end; ++t) {
        real32 dt = (t - x_start) / delta;
        int32 x = t;
        int32 y = parallel_to_axis ? y_start : ((1.0 - dt) * y_start + dt * y_end);
        if (transpose) {
            x = y;
            y = t;
        }
        buf[(y * buffer->window_width) + x] = 0xFFFFFFFF;
    }
}

void _line_sweeping(Drawing_Buffer *buffer, iVector2 small, iVector2 middle, iVector2 big) {
    if (small.y > middle.y)  SWAPVAR(iVector2, small,  middle);
    if (small.y > big.y)     SWAPVAR(iVector2,  small, big);
    if (middle.y > big.y)    SWAPVAR(iVector2, middle,  big);

    draw_line(buffer, small.x,  small.y,  middle.x,  middle.y);
    draw_line(buffer, small.x,  small.y,  big.x,     big.y);
    draw_line(buffer, middle.x, middle.y, big.x,     big.y);

    real32 top_half_distance    = (middle.y - small.y);
    real32 total_distance       = (big.y    - small.y);
    real32 bottom_half_distance = (big.y    - middle.y);

    int32 x0, x1;
    for (int32 y = small.y; y < middle.y; ++y) {
        // Draw top half.
        real32 total_dt    = (y - small.y) / total_distance;
        real32 top_half_dt = (y - small.y)  / top_half_distance;
        x0 = (small.x * (1.0 - top_half_dt)) + (top_half_dt * middle.x);
        x1 = (small.x * (1.0 - total_dt)) + (total_dt * big.x);

        draw_line(buffer, x0, y, x1, y);
    }

    for (int32 y = middle.y; y < big.y; ++y) {
            // Draw bottom Half.
        real32 total_dt       = (y - small.y) / total_distance;
        real32 bottom_half_dt = (y - middle.y) / bottom_half_distance;
        x0 = (middle.x * (1.0 - bottom_half_dt)) + (bottom_half_dt * big.x);
        x1 = (small.x  * (1.0 - total_dt)) + (total_dt * big.x);
        draw_line(buffer, x0, y, x1, y);
    }
}

void draw_triangle(Drawing_Buffer *buffer, iVector2 small, iVector2 middle, iVector2 big) {
    if (USE_LINE_SWEEPING) _line_sweeping(buffer, small, middle, big);
    else                   NOT_IMPLEMENTED;
}
