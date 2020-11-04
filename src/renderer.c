
#define _CUR_BUF(buf) buf->is_drawing_first ? buf->first_buffer : buf->second_buffer

void swap_buffer(Drawing_Buffer *buffer) {
    buffer->is_drawing_first = !buffer->is_drawing_first;
}

void draw_line(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1) {
    ASSERT(0 <= x0 && 0 <= x1 && 0 <= y0 && 0 <= y1);
    ASSERT(x0 + x1 < buffer->window_width);
    ASSERT(y0 + y1 < buffer->window_height);
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
    uint32 parallel_to_axis = y_end == y_start;

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
