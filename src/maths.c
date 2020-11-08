#include "maths.h"

fVector3 fcross_iv3(iVector3 A, iVector3 B) {
    fVector3 result;

    result.x = (real32)((A.y * B.z) - (A.z * B.y));
    result.y = (real32)((A.z * B.x) - (A.x * B.z));
    result.z = (real32)((A.x * B.y) - (A.y * B.x));

    return result;
}

real32 fdot_fv3(fVector3 A, fVector3 B) {
    real32 result = (A.x * B.x) + (A.y * B.y) + (A.z * B.z);
    return result;
}

fVector3 fnormalize_fv3(fVector3 A) {
    real32 distance = sqrtf((A.x * A.x) + (A.y * A.y) + (A.z * A.z));
    if (distance == 0) return fVec3(0, 0, 0);

    A.x /= distance;
    A.y /= distance;
    A.z /= distance;

    return A;
}

fVector3 fcross_fv3(fVector3 A, fVector3 B) {
    fVector3 result;

    result.x = (A.y * B.z) - (A.z * B.y);
    result.y = (A.z * B.x) - (A.x * B.z);
    result.z = (A.x * B.y) - (A.y * B.x);

    return result;
}

// 外積のSIMD (SSE2) 版
// 思ったより遅い
fVector3 fcross_fv3_simd(fVector3 A, fVector3 B) {
    __m128 Ayzx      = _mm_set_ps(0.0, A.x, A.z, A.y);
    __m128 Bzxy      = _mm_set_ps(0.0, B.y, B.x, B.z);
    __m128 cls_left  = _mm_mul_ps(Ayzx, Bzxy);

    __m128 Byzx      = _mm_set_ps(0.0, B.x, B.z, B.y);
    __m128 Azxy      = _mm_set_ps(0.0, A.y, A.x, A.z);
    __m128 cls_right = _mm_mul_ps(Byzx, Azxy);

    __m128 res       = _mm_sub_ps(cls_left, cls_right);

    fVector3 result;
    _mm_store_ps((real32 *)&result, res); // fVector3 is actually padded so it's fine here
    return result;
}

iVector3 icross_iv3(iVector3 A, iVector3 B) {
    iVector3 result;

    result.x = (A.y * B.z) - (A.z * B.y);
    result.y = (A.z * B.x) - (A.x * B.z);
    result.z = (A.x * B.y) - (A.y * B.x);

    return result;
}


iVector3 isub_iv3(iVector3 A, iVector3 B) {
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;

    return A;
}

fVector3 fsub_fv3(fVector3 A, fVector3 B) {
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;

    return A;
}
