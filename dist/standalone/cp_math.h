/**
 * @file cp_math.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_math.
 *
 * In ONE file define:
 *   #define CPLUS_CP_MATH_IMPLEMENTATION
 *   #include "cp_math.h"
 */
#ifndef CPLUS_CP_MATH_H
#define CPLUS_CP_MATH_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <immintrin.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Core types dependency */
#ifndef CPLUS_CORE_H
#define CPLUS_CORE_H
/**
 * @file cp_core.h
 * @brief Foundational types, platform macros, RAII defer, and polymorphic printing.
 */


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <alloca.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Exact-width & ergonomic type aliases
 * ------------------------------------------------------------------------- */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef float     f32;
typedef double    f64;

typedef uint8_t   b8;
typedef uint32_t  b32;

/* -------------------------------------------------------------------------
 * Compiler & Optimization Attributes
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) || defined(__clang__)
    #define CP_INLINE static inline __attribute__((always_inline))
    #define CP_NOINLINE __attribute__((noinline))
    #define CP_LIKELY(x) __builtin_expect(!!(x), 1)
    #define CP_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define CP_ALIGN(n) __attribute__((aligned(n)))
    #define CP_UNUSED(x) ((void)(x))
#else
    #define CP_INLINE static inline
    #define CP_NOINLINE
    #define CP_LIKELY(x) (x)
    #define CP_UNLIKELY(x) (x)
    #define CP_ALIGN(n)
    #define CP_UNUSED(x) ((void)(x))
#endif

#define CP_CACHELINE_SIZE 64
#define CP_CACHELINE CP_ALIGN(CP_CACHELINE_SIZE)

/* Default SIMD alignment */
#define CP_DEFAULT_ALIGNMENT 16
#define CP_SIMD_ALIGNMENT    32

/* Helper macro token pasting */
#define CP_CONCAT_INTERNAL_(a, b) a ## b
#define CP_CONCAT(a, b) CP_CONCAT_INTERNAL_(a, b)

/* -------------------------------------------------------------------------
 * RAII Defer mechanism
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*fn)(void *);
    void *arg;
} cp_defer_t;

CP_INLINE void _cp_defer_cleanup(cp_defer_t *d) {
    if (d->fn) {
        d->fn(d->arg);
    }
}

typedef void (*cp_action_fn)(void);
CP_INLINE void _cp_defer_action_cleanup(cp_action_fn *fn) {
    if (*fn) {
        (*fn)();
    }
}

/**
 * @brief Defer execution of a function with 1 argument until the enclosing scope exits.
 * @param func_ Pointer to function taking 1 pointer argument: void func(void *arg)
 * @param arg_ Pointer passed to func when exiting scope.
 */
#define Defer(func_, arg_) \
    __attribute__((cleanup(_cp_defer_cleanup))) cp_defer_t CP_CONCAT(_cp_defer_, __LINE__) = { \
        .fn = (void (*)(void *))(func_), \
        .arg = (void *)(arg_) \
    }

/**
 * @brief Defer execution of a parameterless function until the enclosing scope exits.
 */
#define DeferAction(func_) \
    __attribute__((cleanup(_cp_defer_action_cleanup))) cp_action_fn CP_CONCAT(_cp_defact_, __LINE__) = (func_)

/* -------------------------------------------------------------------------
 * Anonymous Lambda Functions (Zero Runtime Overhead)
 * Supported natively on GCC via GNU C extension ({ ret_type fn args body fn; }).
 * On Clang / MSVC without nested function syntax, use static functions or
 * zero-overhead iteration macros (VecEach, VecMap).
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) && !defined(__clang__) && !defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__)
#define CP_HAS_LAMBDA 1
/**
 * @brief Define an anonymous inlined lambda function with zero runtime overhead (GCC).
 * @example AlgoSort(arr, len, sizeof(int), Lambda(int, (const void *a, const void *b), { return *(int*)a - *(int*)b; }));
 */
#define Lambda(ret_type, args, body) __extension__ ({ ret_type _cp_lambda args body _cp_lambda; })
#else
#define CP_HAS_LAMBDA 0
#endif

/* -------------------------------------------------------------------------
 * Polymorphic Type-Safe Printing (C11 _Generic)
 * ------------------------------------------------------------------------- */
CP_INLINE void _cp_print_bool(bool v) { fputs(v ? "true" : "false", stdout); }
CP_INLINE void _cp_print_char(char v) { putchar(v); }
CP_INLINE void _cp_print_i64(long long v) { printf("%lld", v); }
CP_INLINE void _cp_print_u64(unsigned long long v) { printf("%llu", v); }
CP_INLINE void _cp_print_f64(double v) { printf("%.6g", v); }
CP_INLINE void _cp_print_str(const char *v) { fputs(v ? v : "(null)", stdout); }
CP_INLINE void _cp_print_ptr(const void *v) { printf("%p", v); }

#define Print(val) _Generic((val), \
    _Bool: _cp_print_bool, \
    char: _cp_print_char, \
    signed char: _cp_print_i64, \
    short: _cp_print_i64, \
    int: _cp_print_i64, \
    long: _cp_print_i64, \
    long long: _cp_print_i64, \
    unsigned char: _cp_print_u64, \
    unsigned short: _cp_print_u64, \
    unsigned int: _cp_print_u64, \
    unsigned long: _cp_print_u64, \
    unsigned long long: _cp_print_u64, \
    float: _cp_print_f64, \
    double: _cp_print_f64, \
    char*: _cp_print_str, \
    const char*: _cp_print_str, \
    default: _cp_print_ptr \
)(val)

#define Println(val) do { Print(val); putchar('\n'); } while(0)
#define PrintF(fmt, ...) printf(fmt, __VA_ARGS__)

/* -------------------------------------------------------------------------
 * Panic & Assertions
 * ------------------------------------------------------------------------- */
#define Panic(msg) do { \
    fprintf(stderr, "\n[CPLUS PANIC] %s:%d: %s\n", __FILE__, __LINE__, msg); \
    abort(); \
} while(0)

#define AssertMsg(cond, msg) do { \
    if (CP_UNLIKELY(!(cond))) { \
        fprintf(stderr, "\n[CPLUS ASSERTION FAILED] %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, msg); \
        abort(); \
    } \
} while(0)

/* -------------------------------------------------------------------------
 * Bit Manipulation Intrinsics
 * ------------------------------------------------------------------------- */
CP_INLINE u32 cp_bit_popcount32(u32 x) { return (u32)__builtin_popcount(x); }
CP_INLINE u32 cp_bit_popcount64(u64 x) { return (u32)__builtin_popcountll(x); }
CP_INLINE u32 cp_bit_clz32(u32 x) { return x == 0 ? 32 : (u32)__builtin_clz(x); }
CP_INLINE u32 cp_bit_clz64(u64 x) { return x == 0 ? 64 : (u32)__builtin_clzll(x); }
CP_INLINE u32 cp_bit_ctz32(u32 x) { return x == 0 ? 32 : (u32)__builtin_ctz(x); }
CP_INLINE u32 cp_bit_ctz64(u64 x) { return x == 0 ? 64 : (u32)__builtin_ctzll(x); }
CP_INLINE u16 cp_bit_bswap16(u16 x) { return __builtin_bswap16(x); }
CP_INLINE u32 cp_bit_bswap32(u32 x) { return __builtin_bswap32(x); }
CP_INLINE u64 cp_bit_bswap64(u64 x) { return __builtin_bswap64(x); }
CP_INLINE u32 cp_bit_rotl32(u32 x, u32 s) { return (x << (s & 31)) | (x >> ((32 - (s & 31)) & 31)); }
CP_INLINE u32 cp_bit_rotr32(u32 x, u32 s) { return (x >> (s & 31)) | (x << ((32 - (s & 31)) & 31)); }
CP_INLINE u64 cp_bit_rotl64(u64 x, u32 s) { return (x << (s & 63)) | (x >> ((64 - (s & 63)) & 63)); }
CP_INLINE u64 cp_bit_rotr64(u64 x, u32 s) { return (x >> (s & 63)) | (x << ((64 - (s & 63)) & 63)); }
CP_INLINE bool cp_bit_is_pow2(u64 x) { return (x != 0) && ((x & (x - 1)) == 0); }
CP_INLINE u64 cp_bit_next_pow2(u64 x) {
    if (x <= 1) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

#define BitPopcount32(x) cp_bit_popcount32(x)
#define BitPopcount64(x) cp_bit_popcount64(x)
#define BitClz32(x)      cp_bit_clz32(x)
#define BitClz64(x)      cp_bit_clz64(x)
#define BitCtz32(x)      cp_bit_ctz32(x)
#define BitCtz64(x)      cp_bit_ctz64(x)
#define BitBswap16(x)    cp_bit_bswap16(x)
#define BitBswap32(x)    cp_bit_bswap32(x)
#define BitBswap64(x)    cp_bit_bswap64(x)
#define BitRotl32(x, s)  cp_bit_rotl32(x, s)
#define BitRotr32(x, s)  cp_bit_rotr32(x, s)
#define BitRotl64(x, s)  cp_bit_rotl64(x, s)
#define BitRotr64(x, s)  cp_bit_rotr64(x, s)
#define BitIsPow2(x)     cp_bit_is_pow2(x)
#define BitNextPow2(x)   cp_bit_next_pow2(x)

typedef struct {
    u32 (*const popcount32)(u32 x);
    u32 (*const popcount64)(u64 x);
    u32 (*const clz32)(u32 x);
    u32 (*const clz64)(u64 x);
    u32 (*const ctz32)(u32 x);
    u32 (*const ctz64)(u64 x);
    u16 (*const bswap16)(u16 x);
    u32 (*const bswap32)(u32 x);
    u64 (*const bswap64)(u64 x);
    u32 (*const rotl32)(u32 x, u32 s);
    u32 (*const rotr32)(u32 x, u32 s);
    u64 (*const rotl64)(u64 x, u32 s);
    u64 (*const rotr64)(u64 x, u32 s);
    bool (*const isPow2)(u64 x);
    u64 (*const nextPow2)(u64 x);
} cp_bit_namespace_t;

static const cp_bit_namespace_t bit = {
    .popcount32 = cp_bit_popcount32,
    .popcount64 = cp_bit_popcount64,
    .clz32 = cp_bit_clz32,
    .clz64 = cp_bit_clz64,
    .ctz32 = cp_bit_ctz32,
    .ctz64 = cp_bit_ctz64,
    .bswap16 = cp_bit_bswap16,
    .bswap32 = cp_bit_bswap32,
    .bswap64 = cp_bit_bswap64,
    .rotl32 = cp_bit_rotl32,
    .rotr32 = cp_bit_rotr32,
    .rotl64 = cp_bit_rotl64,
    .rotr64 = cp_bit_rotr64,
    .isPow2 = cp_bit_is_pow2,
    .nextPow2 = cp_bit_next_pow2,
};

/* -------------------------------------------------------------------------
 * Colored Diagnostic Logging
 * ------------------------------------------------------------------------- */
typedef enum {
    CP_LOG_LEVEL_DEBUG = 0,
    CP_LOG_LEVEL_INFO  = 1,
    CP_LOG_LEVEL_WARN  = 2,
    CP_LOG_LEVEL_ERROR = 3,
    CP_LOG_LEVEL_OFF   = 4,
} cp_log_level_t;

static inline cp_log_level_t *_cp_log_level_ptr(void) {
    static cp_log_level_t lvl = CP_LOG_LEVEL_INFO;
    return &lvl;
}

CP_INLINE void cp_log_set_level(cp_log_level_t lvl) {
    *_cp_log_level_ptr() = lvl;
}

CP_INLINE cp_log_level_t cp_log_get_level(void) {
    return *_cp_log_level_ptr();
}

#define LogSetLevel(lvl) cp_log_set_level(lvl)

#define LogDebug(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_DEBUG) { \
        fprintf(stderr, "\033[90m[DEBUG]\033[0m " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogInfo(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_INFO) { \
        fprintf(stdout, "\033[32m[INFO]\033[0m  " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogWarn(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_WARN) { \
        fprintf(stderr, "\033[33m[WARN]\033[0m  " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogError(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_ERROR) { \
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif
#endif /* CPLUS_CORE_H */

/**
 * @file cp_math.h
 * @brief High-performance linear algebra and 3D math: Vec2, Vec3, Vec4,
 *        and SIMD-accelerated 4x4 Matrices (AVX/SSE vectorized multiplication and inversion).
 */


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

#endif /* CPLUS_CP_MATH_H */

#ifdef CPLUS_CP_MATH_IMPLEMENTATION
/**
 * @file cp_math.c
 * @brief High-performance linear algebra implementation with SSE/AVX hardware SIMD acceleration.
 */

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
#endif /* CPLUS_CP_MATH_IMPLEMENTATION */
