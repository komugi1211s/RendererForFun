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
