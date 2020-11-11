#ifndef _K_MATHS_H
#define _K_MATHS_H

#include <emmintrin.h> // SSE2 SIMD.


#define MATHS_PI 3.14159265358979323846

#define MATHS_MIN(a, b) (((a) > (b)) ? (b) : (a))
#define MATHS_MAX(a, b) (((a) < (b)) ? (b) : (a))
#define MATHS_DEG2RAD(a) ((a) * (MATHS_PI/180));

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

typedef struct BoundingBoxi2 {
    iVector2 min_v;
    iVector2 max_v;
} BoundingBoxi2;

typedef struct BoundingBoxf3 {
    fVector3 min_v;
    fVector3 max_v;
} BoundingBoxf3;


internal inline iVector2
iVec2(int32 x, int32 y) {
    iVector2 result = {0};
    result.x = x;
    result.y = y;
    return result;
}

internal inline iVector3
iVec3(int32 x, int32 y, int32 z) {
    iVector3 result = {0};
    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

internal inline fVector3
fVec3(real32 x, real32 y, real32 z) {
    fVector3 result = {0};
    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

internal inline BoundingBoxi2
BB_iV2_Line(iVector2 one, iVector2 two) {
    BoundingBoxi2 box = {0};

    box.min_v.x = MATHS_MIN(one.x, two.x);
    box.min_v.y = MATHS_MIN(one.y, two.y);

    box.max_v.x = MATHS_MAX(one.x, two.x);
    box.max_v.y = MATHS_MAX(one.y, two.y);

    return box;
}

internal inline BoundingBoxi2
BB_iV2_Triangle(iVector2 one, iVector2 two, iVector2 three) {
    BoundingBoxi2 box = {0};

    box.min_v.x = MATHS_MIN(MATHS_MIN(one.x, two.x), three.x);
    box.min_v.y = MATHS_MIN(MATHS_MIN(one.y, two.y), three.y);
    box.max_v.x = MATHS_MAX(MATHS_MAX(one.x, two.x), three.x);
    box.max_v.y = MATHS_MAX(MATHS_MAX(one.y, two.y), three.y);

    return box;
}

internal inline BoundingBoxf3
BB_fV3(fVector3 one, fVector3 two, fVector3 three) {
    BoundingBoxf3 box = {0};

    box.min_v.x = MATHS_MIN(MATHS_MIN(one.x, two.x), three.x);
    box.min_v.y = MATHS_MIN(MATHS_MIN(one.y, two.y), three.y);
    box.min_v.z = MATHS_MIN(MATHS_MIN(one.z, two.z), three.z);

    box.max_v.x = MATHS_MAX(MATHS_MAX(one.x, two.x), three.x);
    box.max_v.y = MATHS_MAX(MATHS_MAX(one.y, two.y), three.y);
    box.max_v.z = MATHS_MAX(MATHS_MAX(one.z, two.z), three.z);

    return box;
}

internal inline fVector3 fnormalize_fv3(fVector3 A);
internal inline real32 fdot_fv3(fVector3 A, fVector3 B);
internal inline real32 fdot_fv3_simd(fVector3 A, fVector3 B);

internal inline fVector3 fcross_iv3(iVector3 A, iVector3 B);
internal inline fVector3 fcross_fv3(fVector3 A, fVector3 B);
internal inline fVector3 fcross_fv3_simd(fVector3 A, fVector3 B);

internal inline iVector3 icross_iv3(iVector3 A, iVector3 B);
internal inline iVector3 isub_iv3(iVector3 A, iVector3 B);
internal inline fVector3 fadd_fv3(fVector3 A, fVector3 B);
internal inline fVector3 fsub_fv3(fVector3 A, fVector3 B);
internal inline fVector3 fmul_fv3_fscalar(fVector3 A, real32 Scalar);

#endif
