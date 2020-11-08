#ifndef _K_MATHS_H
#define _K_MATHS_H

#include <emmintrin.h> // SSE2 SIMD.

typedef struct iVector2 {
    int32 x, y;
    int32 __pad, __pad2;
} iVector2;

typedef struct iVector3 {
    int32 x, y, z;
    int32 __pad;
} iVector3;

typedef struct fVector3 {
    real32 x, y, z;
    real32 __pad;
} fVector3;

typedef struct BoundingBox {
    iVector2 min_v;
    iVector2 max_v;
} BoundingBox;


iVector2 iVec2(int32 x, int32 y) {
    iVector2 result = {0};
    result.x = x;
    result.y = y;
    return result;
}

iVector3 iVec3(int32 x, int32 y, int32 z) {
    iVector3 result = {0};
    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

fVector3 fVec3(real32 x, real32 y, real32 z) {
    fVector3 result = {0};
    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

#define MATHS_MIN(a, b) ((a > b) ? b : a)
#define MATHS_MAX(a, b) ((a < b) ? b : a)

BoundingBox BB_iV2_Line(iVector2 one, iVector2 two) {
    BoundingBox box;

    box.min_v.x = MATHS_MIN(one.x, two.x);
    box.min_v.y = MATHS_MIN(one.y, two.y);

    box.max_v.x = MATHS_MAX(one.x, two.x);
    box.max_v.y = MATHS_MAX(one.y, two.y);

    return box;
}

BoundingBox BB_iV2_Triangle(iVector2 one, iVector2 two, iVector2 three) {
    BoundingBox box;

    box.min_v.x = MATHS_MIN(MATHS_MIN(one.x, two.x), three.x);
    box.min_v.y = MATHS_MIN(MATHS_MIN(one.y, two.y), three.y);
    box.max_v.x = MATHS_MAX(MATHS_MAX(one.x, two.x), three.x);
    box.max_v.y = MATHS_MAX(MATHS_MAX(one.y, two.y), three.y);

    return box;
}

fVector3 fnormalize_fv3(fVector3 A);
real32 fdot_fv3(fVector3 A, fVector3 B);
fVector3 fcross_iv3(iVector3 A, iVector3 B);

fVector3 fcross_fv3(fVector3 A, fVector3 B);
fVector3 fcross_fv3_simd(fVector3 A, fVector3 B);

iVector3 icross_iv3(iVector3 A, iVector3 B);
iVector3 isub_iv3(iVector3 A, iVector3 B);
fVector3 fsub_fv3(fVector3 A, fVector3 B);

#endif
