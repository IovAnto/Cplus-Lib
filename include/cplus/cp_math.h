/**
 * @file cp_math.h
 * @brief High-performance linear algebra and 3D math: Vec2, Vec3, Vec4,
 *        and SIMD-accelerated 4x4 Matrices (AVX/SSE vectorized multiplication and inversion).
 */

#ifndef CPLUS_MATH_H
#define CPLUS_MATH_H

#include "cp_core.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CP_PI 3.14159265358979323846f
#define CP_DEG2RAD(d) ((d) * (CP_PI / 180.0f))
#define CP_RAD2DEG(r) ((r) * (180.0f / CP_PI))

/* -------------------------------------------------------------------------
 * Scalar Helpers
 * ------------------------------------------------------------------------- */
CP_INLINE f32 cp_clamp(f32 val, f32 min_val, f32 max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

CP_INLINE f32 cp_lerp(f32 a, f32 b, f32 t) {
    return a + t * (b - a);
}

CP_INLINE f32 cp_fast_inv_sqrt(f32 number) {
    union { f32 f; u32 i; } conv = { .f = number };
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f *= (1.5f - (number * 0.5f * conv.f * conv.f));
    return conv.f;
}

#define Clamp(val, min_v, max_v)   cp_clamp(val, min_v, max_v)
#define Lerp(a, b, t)              cp_lerp(a, b, t)
#define MathFastInvSqrt(x)         cp_fast_inv_sqrt(x)

/* -------------------------------------------------------------------------
 * Vector 2D, 3D, 4D
 * ------------------------------------------------------------------------- */
typedef struct {
    f32 x, y;
} cp_vec2_t;

typedef cp_vec2_t Vec2;

typedef struct {
    f32 x, y, z;
} cp_vec3_t;

typedef cp_vec3_t Vec3;

typedef CP_ALIGN(16) struct {
    f32 x, y, z, w;
} cp_vec4_t;

typedef cp_vec4_t Vec4;

CP_INLINE Vec2 cp_vec2(f32 x, f32 y) { return (Vec2){ x, y }; }
CP_INLINE Vec2 cp_vec2_add(Vec2 a, Vec2 b) { return (Vec2){ a.x + b.x, a.y + b.y }; }
CP_INLINE Vec2 cp_vec2_sub(Vec2 a, Vec2 b) { return (Vec2){ a.x - b.x, a.y - b.y }; }
CP_INLINE Vec2 cp_vec2_scale(Vec2 a, f32 s) { return (Vec2){ a.x * s, a.y * s }; }
CP_INLINE f32  cp_vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
CP_INLINE f32  cp_vec2_len(Vec2 a) { return sqrtf(cp_vec2_dot(a, a)); }

CP_INLINE Vec3 cp_vec3(f32 x, f32 y, f32 z) { return (Vec3){ x, y, z }; }
CP_INLINE Vec3 cp_vec3_add(Vec3 a, Vec3 b) { return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z }; }
CP_INLINE Vec3 cp_vec3_sub(Vec3 a, Vec3 b) { return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z }; }
CP_INLINE Vec3 cp_vec3_scale(Vec3 a, f32 s) { return (Vec3){ a.x * s, a.y * s, a.z * s }; }
CP_INLINE f32  cp_vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
CP_INLINE Vec3 cp_vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
CP_INLINE f32  cp_vec3_len(Vec3 a) { return sqrtf(cp_vec3_dot(a, a)); }
CP_INLINE Vec3 cp_vec3_norm(Vec3 a) {
    f32 l = cp_vec3_len(a);
    return l > 0.0f ? cp_vec3_scale(a, 1.0f / l) : a;
}

CP_INLINE Vec4 cp_vec4(f32 x, f32 y, f32 z, f32 w) { return (Vec4){ x, y, z, w }; }
CP_INLINE Vec4 cp_vec4_add(Vec4 a, Vec4 b) { return (Vec4){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
CP_INLINE f32  cp_vec4_dot(Vec4 a, Vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

/* -------------------------------------------------------------------------
 * Matrix 4x4 (Column-major format for SIMD/GPU compatibility)
 * ------------------------------------------------------------------------- */
typedef CP_ALIGN(16) struct {
    f32 m[16];
} cp_mat4_t;

typedef cp_mat4_t Mat4;

Mat4 cp_mat4_identity(void);
Mat4 cp_mat4_translate(f32 tx, f32 ty, f32 tz);
Mat4 cp_mat4_scale(f32 sx, f32 sy, f32 sz);
Mat4 cp_mat4_rotate_x(f32 radians);
Mat4 cp_mat4_rotate_y(f32 radians);
Mat4 cp_mat4_rotate_z(f32 radians);
Mat4 cp_mat4_perspective(f32 fov_rad, f32 aspect, f32 near_z, f32 far_z);
Mat4 cp_mat4_look_at(Vec3 eye, Vec3 center, Vec3 up);

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define CP_HAS_X86_SIMD 1
#endif

CP_INLINE Mat4 cp_mat4_mul(const Mat4 *a, const Mat4 *b) {
    Mat4 res;
#if defined(CP_HAS_X86_SIMD)
    __m128 a_col0 = _mm_loadu_ps(&a->m[0]);
    __m128 a_col1 = _mm_loadu_ps(&a->m[4]);
    __m128 a_col2 = _mm_loadu_ps(&a->m[8]);
    __m128 a_col3 = _mm_loadu_ps(&a->m[12]);

    for (int col = 0; col < 4; col++) {
        __m128 b_col = _mm_loadu_ps(&b->m[col * 4]);
        __m128 e0 = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 e1 = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 e2 = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(2, 2, 2, 2));
        __m128 e3 = _mm_shuffle_ps(b_col, b_col, _MM_SHUFFLE(3, 3, 3, 3));

        __m128 sum = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(a_col0, e0), _mm_mul_ps(a_col1, e1)),
            _mm_add_ps(_mm_mul_ps(a_col2, e2), _mm_mul_ps(a_col3, e3))
        );
        _mm_storeu_ps(&res.m[col * 4], sum);
    }
#else
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            res.m[col * 4 + row] =
                a->m[0 * 4 + row] * b->m[col * 4 + 0] +
                a->m[1 * 4 + row] * b->m[col * 4 + 1] +
                a->m[2 * 4 + row] * b->m[col * 4 + 2] +
                a->m[3 * 4 + row] * b->m[col * 4 + 3];
        }
    }
#endif
    return res;
}

CP_INLINE Vec4 cp_mat4_mul_vec4(const Mat4 *m, Vec4 v) {
    Vec4 res;
#if defined(CP_HAS_X86_SIMD)
    __m128 m0 = _mm_loadu_ps(&m->m[0]);
    __m128 m1 = _mm_loadu_ps(&m->m[4]);
    __m128 m2 = _mm_loadu_ps(&m->m[8]);
    __m128 m3 = _mm_loadu_ps(&m->m[12]);

    __m128 vx = _mm_set1_ps(v.x);
    __m128 vy = _mm_set1_ps(v.y);
    __m128 vz = _mm_set1_ps(v.z);
    __m128 vw = _mm_set1_ps(v.w);

    __m128 sum = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(m0, vx), _mm_mul_ps(m1, vy)),
        _mm_add_ps(_mm_mul_ps(m2, vz), _mm_mul_ps(m3, vw))
    );
    _mm_storeu_ps(&res.x, sum);
#else
    res.x = m->m[0] * v.x + m->m[4] * v.y + m->m[8]  * v.z + m->m[12] * v.w;
    res.y = m->m[1] * v.x + m->m[5] * v.y + m->m[9]  * v.z + m->m[13] * v.w;
    res.z = m->m[2] * v.x + m->m[6] * v.y + m->m[10] * v.z + m->m[14] * v.w;
    res.w = m->m[3] * v.x + m->m[7] * v.y + m->m[11] * v.z + m->m[15] * v.w;
#endif
    return res;
}

b32  cp_mat4_invert(const Mat4 *src, Mat4 *dst);
Mat4 cp_mat4_transpose(const Mat4 *m);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define Mat4Identity()                 cp_mat4_identity()
#define Mat4Translate(x, y, z)         cp_mat4_translate(x, y, z)
#define Mat4Scale(x, y, z)             cp_mat4_scale(x, y, z)
#define Mat4RotateX(rad)               cp_mat4_rotate_x(rad)
#define Mat4RotateY(rad)               cp_mat4_rotate_y(rad)
#define Mat4RotateZ(rad)               cp_mat4_rotate_z(rad)
#define Mat4Perspective(fov, asp, n, f) cp_mat4_perspective(fov, asp, n, f)
#define Mat4LookAt(eye, center, up)    cp_mat4_look_at(eye, center, up)

#define Mat4Mul(a, b)                  cp_mat4_mul(a, b)
#define Mat4MulVec4(m, v)              cp_mat4_mul_vec4(m, v)
#define Mat4Invert(src, dst)           cp_mat4_invert(src, dst)
#define Mat4Transpose(m)               cp_mat4_transpose(m)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    Mat4 (*const identity)(void);
    Mat4 (*const translate)(f32 tx, f32 ty, f32 tz);
    Mat4 (*const scale)(f32 sx, f32 sy, f32 sz);
    Mat4 (*const rotateX)(f32 radians);
    Mat4 (*const rotateY)(f32 radians);
    Mat4 (*const rotateZ)(f32 radians);
    Mat4 (*const perspective)(f32 fov_rad, f32 aspect, f32 near_z, f32 far_z);
    Mat4 (*const lookAt)(Vec3 eye, Vec3 center, Vec3 up);
    Mat4 (*const mul)(const Mat4 *a, const Mat4 *b);
    Vec4 (*const mulVec4)(const Mat4 *m, Vec4 v);
    b32  (*const invert)(const Mat4 *src, Mat4 *dst);
    Mat4 (*const transpose)(const Mat4 *m);
    f32  (*const clamp)(f32 val, f32 min_val, f32 max_val);
    f32  (*const lerp)(f32 a, f32 b, f32 t);
    f32  (*const fastInvSqrt)(f32 x);
} cp_math_namespace_t;

static const cp_math_namespace_t math = {
    .identity = cp_mat4_identity,
    .translate = cp_mat4_translate,
    .scale = cp_mat4_scale,
    .rotateX = cp_mat4_rotate_x,
    .rotateY = cp_mat4_rotate_y,
    .rotateZ = cp_mat4_rotate_z,
    .perspective = cp_mat4_perspective,
    .lookAt = cp_mat4_look_at,
    .mul = cp_mat4_mul,
    .mulVec4 = cp_mat4_mul_vec4,
    .invert = cp_mat4_invert,
    .transpose = cp_mat4_transpose,
    .clamp = cp_clamp,
    .lerp = cp_lerp,
    .fastInvSqrt = cp_fast_inv_sqrt,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_MATH_H */
