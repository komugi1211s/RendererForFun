typedef struct {
    int32 window_width;
    int32 window_height;

    int32 depth;
    bool32 is_drawing_first;

    uint32 *first_buffer;
    uint32 *second_buffer;
} Drawing_Buffer;
