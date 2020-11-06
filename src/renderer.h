typedef struct {
    int32 window_width;
    int32 window_height;

    int32 depth;
    bool32 is_drawing_first;

    uint32 *first_buffer;
    uint32 *second_buffer;
} Drawing_Buffer;

typedef struct {
    int32 x, y;
} iVector2;

typedef struct {
    int32 x, y, z;
} iVector3;

typedef struct {
    iVector2 min_v;
    iVector2 max_v;
} BoundingBox;


iVector2 iVec2(int32 x, int32 y) {
    iVector2 result = {0};
    result.x = x;
    result.y = y;
    return result;
}

BoundingBox BB(iVector2 one, iVector2 two) {
    BoundingBox box;
    box.min_v.x = one.x > two.x ? two.x : one.x;
    box.min_v.y = one.y > two.y ? two.y : one.y;
    box.max_v.x = one.x < two.x ? two.x : one.x;
    box.max_v.y = one.y < two.y ? two.y : one.y;

    return box;
}

