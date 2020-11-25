#ifndef _K_MATH_H
#define _K_MATH_H

#include <emmintrin.h> // SSE2 SIMD.


#define FP_PI 3.14159265358979323846

#define FP_MIN(a, b) (((a) > (b)) ? (b) : (a))
#define FP_MAX(a, b) (((a) < (b)) ? (b) : (a))

#define FP_CLAMP(x, lower, upper) (FP_MAX((lower), FP_MIN((upper), (x))))
#define FP_CLAMP01(x) FP_CLAMP(x, 0, 1)

#define FP_RAD2DEG(a) ((a) * (180/FP_PI))
#define FP_DEG2RAD(a) ((a) * (FP_PI/180))

typedef struct iVector2 {
    int32 x, y;
    int32 __pad, __pad2;
} iVector2;

typedef struct iVector3 {
    int32 x, y, z, __pad;
} iVector3;

typedef union fVector3 {
    struct { real32 x, y, z; };
    real32 e[3];
} fVector3;


typedef union fVector4 {
    struct {
        union {
            struct { real32 x, y, z; };
            fVector3 xyz;
        };
        real32 w;
    };
    real32 e[4];
} fVector4;


typedef struct BoundingBoxi2 {
    iVector2 min_v;
    iVector2 max_v;
} BoundingBoxi2;


typedef struct BoundingBoxf3 {
    fVector3 min_v;
    fVector3 max_v;
} BoundingBoxf3;


typedef union fMat4x4 {
    struct { fVector4 x, y, z, w; };
    struct { real32 rc[4][4]; };
    struct { real32 col[4]; } row[4];
    real32 e[16];
} fMat4x4;


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

internal inline fMat4x4
fMat4Identity() {
    fMat4x4 result = {0};
    result.row[0].col[0] = 1.0f;
    result.row[1].col[1] = 1.0f;
    result.row[2].col[2] = 1.0f;
    result.row[3].col[3] = 1.0f;
    return result;
}

internal inline BoundingBoxi2
BB_iV2_Line(iVector2 one, iVector2 two) {
    BoundingBoxi2 box = {0};

    box.min_v.x = FP_MIN(one.x, two.x);
    box.min_v.y = FP_MIN(one.y, two.y);

    box.max_v.x = FP_MAX(one.x, two.x);
    box.max_v.y = FP_MAX(one.y, two.y);

    return box;
}

internal inline BoundingBoxi2
BB_iV2_Triangle(iVector2 one, iVector2 two, iVector2 three) {
    BoundingBoxi2 box = {0};

    box.min_v.x = FP_MIN(FP_MIN(one.x, two.x), three.x);
    box.min_v.y = FP_MIN(FP_MIN(one.y, two.y), three.y);
    box.max_v.x = FP_MAX(FP_MAX(one.x, two.x), three.x);
    box.max_v.y = FP_MAX(FP_MAX(one.y, two.y), three.y);

    return box;
}

internal inline BoundingBoxf3
BB_fV3(fVector3 one, fVector3 two, fVector3 three) {
    BoundingBoxf3 box = {0};

    box.min_v.x = FP_MIN(FP_MIN(one.x, two.x), three.x);
    box.min_v.y = FP_MIN(FP_MIN(one.y, two.y), three.y);
    box.min_v.z = FP_MIN(FP_MIN(one.z, two.z), three.z);

    box.max_v.x = FP_MAX(FP_MAX(one.x, two.x), three.x);
    box.max_v.y = FP_MAX(FP_MAX(one.y, two.y), three.y);
    box.max_v.z = FP_MAX(FP_MAX(one.z, two.z), three.z);

    return box;
}

internal fMat4x4 fMat4Lookat(fVector3 position, fVector3 target, fVector3 up);
internal fMat4x4 fMat4Perspective(real32 aspect_ratio, real32 fovy, real32 z_near, real32 z_far);
internal fMat4x4 fMat4WorldToScreen(real32 width, real32 height, real32 z_near, real32 z_far);

internal inline fVector3 fneg_fv3(fVector3 A);
internal inline fVector3 fadd_fv3(fVector3 A, fVector3 B);
internal inline fVector3 fsub_fv3(fVector3 A, fVector3 B);
internal inline fVector3 fmul_fv3_fscalar(fVector3 A, real32 Scalar);

internal inline fVector3 fnormalize_fv3(fVector3 A);
internal inline real32   fdot_fv3(fVector3 A, fVector3 B);
internal inline real32   fdot_fv3_simd(fVector3 A, fVector3 B);
internal inline fVector3 fcross_iv3(iVector3 A, iVector3 B);
internal inline fVector3 fcross_fv3(fVector3 A, fVector3 B);
internal inline fVector3 fcross_fv3_simd(fVector3 A, fVector3 B);

internal inline iVector3 icross_iv3(iVector3 A, iVector3 B);
internal inline iVector3 isub_iv3(iVector3 A, iVector3 B);

internal inline fMat4x4  fmul_fmat4x4(fMat4x4 A, fMat4x4 B);
internal inline fVector4 fmul_fmat4x4_fv4(fMat4x4 A, fVector4 B);

internal inline real32   fdeterminant_triangle_fv3(fVector3 a, fVector3 b, fVector3 c);

#endif // _K_MATH_H
