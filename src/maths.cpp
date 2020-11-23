#include "maths.h"

internal inline fVector3
fcross_iv3(iVector3 A, iVector3 B) {
    fVector3 result;

    result.x = (real32)((A.y * B.z) - (A.z * B.y));
    result.y = (real32)((A.z * B.x) - (A.x * B.z));
    result.z = (real32)((A.x * B.y) - (A.y * B.x));

    return result;
}

internal inline real32
fdot_fv3(fVector3 A, fVector3 B) {
    real32 result = (A.x * B.x) + (A.y * B.y) + (A.z * B.z);
    return result;
}

fVector3 fnormalize_fv3(fVector3 A) {
    real32 distance = sqrtf((A.x * A.x) + (A.y * A.y) + (A.z * A.z));

    if (distance == 0) {
        A.x = 0;
        A.y = 0;
        A.z = 0;
    } else {
        A.x /= distance;
        A.y /= distance;
        A.z /= distance;
    }

    return A;
}

internal fMat4x4
fMat4Lookat(fVector3 position, fVector3 target, fVector3 up) {
    fVector3 x, y, z;

    z = fnormalize_fv3(target);
    x = fnormalize_fv3(fcross_fv3(up, z));
    y = fnormalize_fv3(fcross_fv3(z, x));

    fMat4x4 result = {0};

    result.x.xyz = x;
    result.y.xyz = y;
    result.z.xyz = z;

    result.row[0].col[3] = -fdot_fv3(x, position);
    result.row[1].col[3] = -fdot_fv3(y, position);
    result.row[2].col[3] = -fdot_fv3(z, position);
    result.row[3].col[3] = 1.0;

    return result;
}

internal fMat4x4
fMat4Perspective(real32 aspect_ratio, real32 fov_y, real32 z_near, real32 z_far) {
    real32 tan_fov = tanf(FP_DEG2RAD(fov_y) / 2);

    fMat4x4 result = {0};
    result.row[0].col[0] = 1.0 / (aspect_ratio * tan_fov);
    result.row[1].col[1] = 1.0 / tan_fov;
    result.row[2].col[2] = -(z_far + z_near) / (z_far - z_near);
    result.row[2].col[3] = (2.0 * z_far * z_near) / (z_near - z_far);
    result.row[3].col[2] = -1.0;

    return result;
}

internal fMat4x4
fMat4WorldToScreen(real32 width, real32 height, real32 depth) {
    fMat4x4 result = {0};

    result.row[0].col[0] = width / 2;
    result.row[1].col[1] = -height / 2;
    result.row[2].col[2] = depth / 2;

    result.row[0].col[3] = width / 2;
    result.row[1].col[3] = height / 2;
    result.row[2].col[3] = depth / 2;

    result.row[3].col[3] = 1.0;
    return result;
}

internal fMat4x4
fMat4ScreenToWorld(real32 width, real32 height, real32 depth) {
    fMat4x4 result = {0};

    result.row[0].col[0] = 

    result.row[3].col[3] = 1.0;
    return result;
}

internal inline fVector3
fneg_fv3(fVector3 A) {
    A.x = -A.x;
    A.y = -A.y;
    A.z = -A.z;

    return A;
}

internal inline fVector3
fcross_fv3(fVector3 A, fVector3 B) {
    fVector3 result;

    result.x = (A.y * B.z) - (A.z * B.y);
    result.y = (A.z * B.x) - (A.x * B.z);
    result.z = (A.x * B.y) - (A.y * B.x);

    return result;
}

// 内積のSIMD (SSE2) 版
internal inline real32
fdot_fv3_simd(fVector3 A, fVector3 B) {
    __m128 left  = _mm_set_ps(0.0, A.z, A.y, A.x);
    __m128 right = _mm_set_ps(0.0, B.z, B.y, B.x);
    __m128 muled = _mm_mul_ps(left, right);

    ALIGN16 fVector4 result;
    _mm_store_ps((real32 *)&result, muled);
    return result.x + result.y + result.z;
}

// 外積のSIMD (SSE2) 版
internal inline fVector3
fcross_fv3_simd(fVector3 A, fVector3 B) {
    __m128 Ayzx      = _mm_set_ps(0.0, A.x, A.z, A.y);
    __m128 Bzxy      = _mm_set_ps(0.0, B.y, B.x, B.z);
    __m128 cls_left  = _mm_mul_ps(Ayzx, Bzxy);

    __m128 Byzx      = _mm_set_ps(0.0, B.x, B.z, B.y);
    __m128 Azxy      = _mm_set_ps(0.0, A.y, A.x, A.z);
    __m128 cls_right = _mm_mul_ps(Byzx, Azxy);

    __m128 res       = _mm_sub_ps(cls_left, cls_right);

    ALIGN16 fVector4 result;
    _mm_store_ps((real32 *)&result, res);
    return result.xyz;
}

internal inline iVector3
icross_iv3(iVector3 A, iVector3 B) {
    iVector3 result;

    result.x = (A.y * B.z) - (A.z * B.y);
    result.y = (A.z * B.x) - (A.x * B.z);
    result.z = (A.x * B.y) - (A.y * B.x);

    return result;
}

internal inline iVector3
isub_iv3(iVector3 A, iVector3 B) {
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;

    return A;
}

internal inline fVector3
fadd_fv3(fVector3 A, fVector3 B) {
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;

    return A;
}

internal inline fVector3
fsub_fv3(fVector3 A, fVector3 B) {
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;

    return A;
}

internal inline fVector3
fmul_fv3_fscalar(fVector3 A, real32 Scalar) {
    A.x *= Scalar;
    A.y *= Scalar;
    A.z *= Scalar;

    return A;
}

internal inline fMat4x4
fmul_fmat4x4(fMat4x4 A, fMat4x4 B) {
    fMat4x4 result = {0};
    for (int32 i = 0; i < 4; ++i) {
        for (int32 j = 0; j < 4; ++j) {
            for (int32 k = 0; k < 4; ++k) {
                result.row[i].col[j] += A.row[i].col[k] * B.row[k].col[j];
            }
        }
    }

    return result;
}

internal inline fVector4
fmul_fmat4x4_fv4(fMat4x4 A, fVector4 B) {
    fVector4 result = {0};

    result.x = (A.row[0].col[0] * B.x) + (A.row[0].col[1] * B.y) + (A.row[0].col[2] * B.z) + (A.row[0].col[3] * B.w);
    result.y = (A.row[1].col[0] * B.x) + (A.row[1].col[1] * B.y) + (A.row[1].col[2] * B.z) + (A.row[1].col[3] * B.w);
    result.z = (A.row[2].col[0] * B.x) + (A.row[2].col[1] * B.y) + (A.row[2].col[2] * B.z) + (A.row[2].col[3] * B.w);
    result.w = (A.row[3].col[0] * B.x) + (A.row[3].col[1] * B.y) + (A.row[3].col[2] * B.z) + (A.row[3].col[3] * B.w);

    return result;
}

internal inline fVector4
fmul_fv4_fmat4x4(fVector4 A, fMat4x4 B) {
    fVector4 result = {0};

    result.x = (A.x * B.row[0].col[0]) + (A.y * B.row[1].col[0]) + (A.z * B.row[2].col[0]) + (A.w * B.row[3].col[0]);
    result.y = (A.x * B.row[0].col[1]) + (A.y * B.row[1].col[1]) + (A.z * B.row[2].col[1]) + (A.w * B.row[3].col[1]);
    result.z = (A.x * B.row[0].col[2]) + (A.y * B.row[1].col[2]) + (A.z * B.row[2].col[2]) + (A.w * B.row[3].col[2]);
    result.w = (A.x * B.row[0].col[3]) + (A.y * B.row[1].col[3]) + (A.z * B.row[2].col[3]) + (A.w * B.row[3].col[3]);

    return result;
}

internal inline real32
fdeterminant_triangle_fv3(fVector3 &a, fVector3 &b, fVector3 &c) {
    return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}

internal inline fVector3
fbarycentric_fv3(fVector3 &A, fVector3 &B, fVector3 &C, fVector3 &P) {
    // 重心座標を計算する
    // ABCの中の点Pを求める際、 ABP / ABC, ACP / ABC, BCP / ABC をそれぞれ割合として求め、
    // 総和が1となった場合返す
    // なおこの際頂点はA->B->Cの順に反時計回りに回っている

    fVector3 result = {0};

    real32 ABx = B.x - A.x;
    real32 ACx = C.x - A.x;
    real32 ABy = B.y - A.y;
    real32 ACy = C.y - A.y;
    
    real32 det_t = (ABx * ACy) - (ACx * ABy);
    if (fabs(det_t) < 1) {
        result.x = -1;
        return result;
    }

    real32 APx = P.x - A.x;
    real32 APy = P.y - A.y;

    real32 acap = (APx * ACy - ACx * APy) / det_t; // multiply with B
    real32 abap = (ABx * APy - APx * ABy) / det_t; // multiply with C
    real32 bcap = 1.0 - acap - abap;       // multiply with A

    // NOTE(fuzzy):
    // ACP,ABPの面積をABCの面積で割って1-ACP/ABC-ABP/ABCした時結果が0以下だった場合、
    // 明らかに点が範囲の外にある事となる
    if (bcap < 0) {
        result.x = -1;
        return result;
    }

    result.x = bcap; // A
    result.y = acap; // B
    result.z = abap; // C
    return result;
}
