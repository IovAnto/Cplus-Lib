/**
 * @file cp_math.c
 * @brief High-performance linear algebra implementation with SSE/AVX hardware SIMD acceleration.
 */

#include "cplus/cp_math.h"
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define CP_HAS_X86_SIMD 1
#endif

/* -------------------------------------------------------------------------
 * Basic Matrix Initializers
 * ------------------------------------------------------------------------- */
Mat4 cp_mat4_identity(void) {
    Mat4 res = {0};
    res.m[0]  = 1.0f;
    res.m[5]  = 1.0f;
    res.m[10] = 1.0f;
    res.m[15] = 1.0f;
    return res;
}

Mat4 cp_mat4_translate(f32 tx, f32 ty, f32 tz) {
    Mat4 res = cp_mat4_identity();
    res.m[12] = tx;
    res.m[13] = ty;
    res.m[14] = tz;
    return res;
}

Mat4 cp_mat4_scale(f32 sx, f32 sy, f32 sz) {
    Mat4 res = {0};
    res.m[0]  = sx;
    res.m[5]  = sy;
    res.m[10] = sz;
    res.m[15] = 1.0f;
    return res;
}

Mat4 cp_mat4_rotate_x(f32 rad) {
    Mat4 res = cp_mat4_identity();
    f32 c = cosf(rad);
    f32 s = sinf(rad);
    res.m[5]  = c;
    res.m[6]  = s;
    res.m[9]  = -s;
    res.m[10] = c;
    return res;
}

Mat4 cp_mat4_rotate_y(f32 rad) {
    Mat4 res = cp_mat4_identity();
    f32 c = cosf(rad);
    f32 s = sinf(rad);
    res.m[0]  = c;
    res.m[2]  = -s;
    res.m[8]  = s;
    res.m[10] = c;
    return res;
}

Mat4 cp_mat4_rotate_z(f32 rad) {
    Mat4 res = cp_mat4_identity();
    f32 c = cosf(rad);
    f32 s = sinf(rad);
    res.m[0] = c;
    res.m[1] = s;
    res.m[4] = -s;
    res.m[5] = c;
    return res;
}

Mat4 cp_mat4_perspective(f32 fov_rad, f32 aspect, f32 near_z, f32 far_z) {
    Mat4 res = {0};
    f32 tan_half_fov = tanf(fov_rad / 2.0f);
    res.m[0]  = 1.0f / (aspect * tan_half_fov);
    res.m[5]  = 1.0f / tan_half_fov;
    res.m[10] = -(far_z + near_z) / (far_z - near_z);
    res.m[11] = -1.0f;
    res.m[14] = -(2.0f * far_z * near_z) / (far_z - near_z);
    return res;
}

Mat4 cp_mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = cp_vec3_norm(cp_vec3_sub(center, eye));
    Vec3 s = cp_vec3_norm(cp_vec3_cross(f, up));
    Vec3 u = cp_vec3_cross(s, f);

    Mat4 res = cp_mat4_identity();
    res.m[0]  = s.x;
    res.m[4]  = s.y;
    res.m[8]  = s.z;
    res.m[1]  = u.x;
    res.m[5]  = u.y;
    res.m[9]  = u.z;
    res.m[2]  = -f.x;
    res.m[6]  = -f.y;
    res.m[10] = -f.z;
    res.m[12] = -cp_vec3_dot(s, eye);
    res.m[13] = -cp_vec3_dot(u, eye);
    res.m[14] =  cp_vec3_dot(f, eye);
    return res;
}

/* -------------------------------------------------------------------------
 * Matrix Inversion (Analytic 4x4 Inversion with Determinant)
 * ------------------------------------------------------------------------- */
b32 cp_mat4_invert(const Mat4 *src, Mat4 *dst) {
    const f32 *s = src->m;
    f32 *d = dst->m;

    f32 inv[16];

    inv[0] = s[5]  * s[10] * s[15] - 
             s[5]  * s[11] * s[14] - 
             s[9]  * s[6]  * s[15] + 
             s[9]  * s[7]  * s[14] +
             s[13] * s[6]  * s[11] - 
             s[13] * s[7]  * s[10];

    inv[4] = -s[4]  * s[10] * s[15] + 
              s[4]  * s[11] * s[14] + 
              s[8]  * s[6]  * s[15] - 
              s[8]  * s[7]  * s[14] - 
              s[12] * s[6]  * s[11] + 
              s[12] * s[7]  * s[10];

    inv[8] = s[4]  * s[9] * s[15] - 
             s[4]  * s[11] * s[13] - 
             s[8]  * s[5] * s[15] + 
             s[8]  * s[7] * s[13] + 
             s[12] * s[5] * s[11] - 
             s[12] * s[7] * s[9];

    inv[12] = -s[4]  * s[9] * s[14] + 
               s[4]  * s[10] * s[13] +
               s[8]  * s[5] * s[14] - 
               s[8]  * s[6] * s[13] - 
               s[12] * s[5] * s[10] + 
               s[12] * s[6] * s[9];

    inv[1] = -s[1]  * s[10] * s[15] + 
              s[1]  * s[11] * s[14] + 
              s[9]  * s[2] * s[15] - 
              s[9]  * s[3] * s[14] - 
              s[13] * s[2] * s[11] + 
              s[13] * s[3] * s[10];

    inv[5] = s[0]  * s[10] * s[15] - 
             s[0]  * s[11] * s[14] - 
             s[8]  * s[2] * s[15] + 
             s[8]  * s[3] * s[14] + 
             s[12] * s[2] * s[11] - 
             s[12] * s[3] * s[10];

    inv[9] = -s[0]  * s[9] * s[15] + 
              s[0]  * s[11] * s[13] + 
              s[8]  * s[1] * s[15] - 
              s[8]  * s[3] * s[13] - 
              s[12] * s[1] * s[11] + 
              s[12] * s[3] * s[9];

    inv[13] = s[0]  * s[9] * s[14] - 
              s[0]  * s[10] * s[13] - 
              s[8]  * s[1] * s[14] + 
              s[8]  * s[2] * s[13] + 
              s[12] * s[1] * s[10] - 
              s[12] * s[2] * s[9];

    inv[2] = s[1]  * s[6] * s[15] - 
             s[1]  * s[7] * s[14] - 
             s[5]  * s[2] * s[15] + 
             s[5]  * s[3] * s[14] + 
             s[13] * s[2] * s[7] - 
             s[13] * s[3] * s[6];

    inv[6] = -s[0]  * s[6] * s[15] + 
              s[0]  * s[7] * s[14] + 
              s[4]  * s[2] * s[15] - 
              s[4]  * s[3] * s[14] - 
              s[12] * s[2] * s[7] + 
              s[12] * s[3] * s[6];

    inv[10] = s[0]  * s[5] * s[15] - 
              s[0]  * s[7] * s[13] - 
              s[4]  * s[1] * s[15] + 
              s[4]  * s[3] * s[13] + 
              s[12] * s[1] * s[7] - 
              s[12] * s[3] * s[5];

    inv[14] = -s[0]  * s[5] * s[14] + 
               s[0]  * s[6] * s[13] + 
               s[4]  * s[1] * s[14] - 
               s[4]  * s[2] * s[13] - 
               s[12] * s[1] * s[6] + 
               s[12] * s[2] * s[5];

    inv[3] = -s[1] * s[6] * s[11] + 
              s[1] * s[7] * s[10] + 
              s[5] * s[2] * s[11] - 
              s[5] * s[3] * s[10] - 
              s[9] * s[2] * s[7] + 
              s[9] * s[3] * s[6];

    inv[7] = s[0] * s[6] * s[11] - 
             s[0] * s[7] * s[10] - 
             s[4] * s[2] * s[11] + 
             s[4] * s[3] * s[10] + 
             s[8] * s[2] * s[7] - 
             s[8] * s[3] * s[6];

    inv[11] = -s[0] * s[5] * s[11] + 
               s[0] * s[7] * s[9] + 
               s[4] * s[1] * s[11] - 
               s[4] * s[3] * s[9] - 
               s[8] * s[1] * s[7] + 
               s[8] * s[3] * s[5];

    inv[15] = s[0] * s[5] * s[10] - 
              s[0] * s[6] * s[9] - 
              s[4] * s[1] * s[10] + 
              s[4] * s[2] * s[9] + 
              s[8] * s[1] * s[6] - 
              s[8] * s[2] * s[5];

    f32 det = s[0] * inv[0] + s[1] * inv[4] + s[2] * inv[8] + s[3] * inv[12];
    if (fabsf(det) < 1e-8f) return false;

    f32 inv_det = 1.0f / det;
    for (int i = 0; i < 16; i++) {
        d[i] = inv[i] * inv_det;
    }
    return true;
}

Mat4 cp_mat4_transpose(const Mat4 *m) {
    Mat4 res;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            res.m[col * 4 + row] = m->m[row * 4 + col];
        }
    }
    return res;
}
