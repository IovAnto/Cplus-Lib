/**
 * @file cp_str.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_str.
 *
 * In ONE file define:
 *   #define CPLUS_CP_STR_IMPLEMENTATION
 *   #include "cp_str.h"
 */
#ifndef CPLUS_CP_STR_H
#define CPLUS_CP_STR_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
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
 * @file cp_str.h
 * @brief Zero-allocation StringView and high-performance Arena-backed String Builder.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * String View (Zero-allocation slice)
 * ------------------------------------------------------------------------- */
typedef struct cp_strview {
    const char *data;
    usize len;
} cp_strview_t;

typedef cp_strview_t StrView;

#define StrViewLit(str_literal) ((cp_strview_t){ (str_literal), sizeof(str_literal) - 1 })
#define S(str_literal)          StrViewLit(str_literal)

cp_strview_t cp_sv_from_c(const char *cstr);
cp_strview_t cp_sv_from_parts(const char *data, usize len);
cp_strview_t cp_sv_sub(cp_strview_t sv, usize start, usize len);
cp_strview_t cp_sv_trim_left(cp_strview_t sv);
cp_strview_t cp_sv_trim_right(cp_strview_t sv);
cp_strview_t cp_sv_trim(cp_strview_t sv);

b32 cp_sv_equals(cp_strview_t a, cp_strview_t b);
b32 cp_sv_equals_c(cp_strview_t a, const char *cstr);
b32 cp_sv_starts_with(cp_strview_t sv, cp_strview_t prefix);
b32 cp_sv_ends_with(cp_strview_t sv, cp_strview_t suffix);
isize cp_sv_find(cp_strview_t sv, cp_strview_t needle);
isize cp_sv_find_char(cp_strview_t sv, char c);

/* Splitting into array of StrView on arena */
usize cp_sv_split(cp_strview_t sv, char delimiter, cp_strview_t *out_views, usize max_views);
cp_strview_t *cp_sv_split_arena(cp_strview_t sv, char delimiter, cp_arena_t *arena, usize *out_count);

/* -------------------------------------------------------------------------
 * Dynamic String / String Builder
 * ------------------------------------------------------------------------- */
typedef struct cp_string {
    char *data;
    usize len;
    usize cap;
    cp_arena_t *arena;
} cp_string_t;

typedef cp_string_t Str;
typedef cp_string_t String;

cp_string_t cp_str_new(cp_arena_t *arena, usize initial_capacity);
cp_string_t cp_str_from_sv(cp_arena_t *arena, cp_strview_t sv);
cp_string_t cp_str_from_c(cp_arena_t *arena, const char *cstr);

void cp_str_append_char(cp_string_t *str, char c);
void cp_str_append_sv(cp_string_t *str, cp_strview_t sv);
void cp_str_append_c(cp_string_t *str, const char *cstr);
void cp_str_append_f(cp_string_t *str, const char *fmt, ...);

cp_string_t cp_str_format(cp_arena_t *arena, const char *fmt, ...);
cp_string_t cp_str_format_scratch(const char *fmt, ...);

cp_strview_t cp_str_to_sv(const cp_string_t *str);
cp_string_t cp_str_join(cp_arena_t *arena, const cp_strview_t *views, usize count, cp_strview_t separator);
cp_string_t cp_str_replace(cp_arena_t *arena, cp_strview_t src, cp_strview_t target, cp_strview_t replacement);

/* -------------------------------------------------------------------------
 * Base64 Encoding & Decoding
 * ------------------------------------------------------------------------- */
i64 cp_base64_encode(const void *src, usize src_len, char *dst, usize dst_cap);
i64 cp_base64_decode(const char *src, usize src_len, void *dst, usize dst_cap);
cp_strview_t cp_base64_encode_arena(const void *src, usize src_len, cp_arena_t *arena);
void *cp_base64_decode_arena(const char *src, usize src_len, cp_arena_t *arena, usize *out_len);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define StrFromC(cstr)                 cp_sv_from_c(cstr)
#define StrFromParts(data, len)        cp_sv_from_parts(data, len)
#define StrSub(sv, start, len)         cp_sv_sub(sv, start, len)
#define StrTrim(sv)                    cp_sv_trim(sv)
#define StrTrimLeft(sv)                cp_sv_trim_left(sv)
#define StrTrimRight(sv)               cp_sv_trim_right(sv)

#define StrEquals(a, b)                cp_sv_equals(a, b)
#define StrEqualsC(a, cstr)            cp_sv_equals_c(a, cstr)
#define StrStartsWith(sv, prefix)      cp_sv_starts_with(sv, prefix)
#define StrEndsWith(sv, suffix)        cp_sv_ends_with(sv, suffix)
#define StrFind(sv, needle)            cp_sv_find(sv, needle)
#define StrFindChar(sv, c)             cp_sv_find_char(sv, c)

#define StrSplit(sv, delim, out, max)  cp_sv_split(sv, delim, out, max)
#define StrSplitArena(sv, d, a, cnt)   cp_sv_split_arena(sv, d, a, cnt)

#define StrNew(arena, cap)             cp_str_new(arena, cap)
#define StrLen(str)                    ((str)->len)
#define StrAppend(str, sv)             cp_str_append_sv(str, sv)
#define StrAppendC(str, cstr)          cp_str_append_c(str, cstr)
#define StrFormat(arena, fmt, ...)     cp_str_format(arena, fmt, __VA_ARGS__)
#define StrFormatScratch(fmt, ...)     cp_str_format_scratch(fmt, __VA_ARGS__)
#define StrJoin(arena, v, cnt, sep)    cp_str_join(arena, v, cnt, sep)
#define StrReplace(arena, s, t, r)     cp_str_replace(arena, s, t, r)

#define Base64Encode(src, len, dst, cap)    cp_base64_encode(src, len, dst, cap)
#define Base64Decode(src, len, dst, cap)    cp_base64_decode(src, len, dst, cap)
#define Base64EncodeArena(src, len, arena)  cp_base64_encode_arena(src, len, arena)
#define Base64DecodeArena(src, l, a, out_l) cp_base64_decode_arena(src, l, a, out_l)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_strview_t (*const fromC)(const char *cstr);
    cp_strview_t (*const sub)(cp_strview_t sv, usize start, usize len);
    cp_strview_t (*const trim)(cp_strview_t sv);
    b32 (*const equals)(cp_strview_t a, cp_strview_t b);
    b32 (*const startsWith)(cp_strview_t sv, cp_strview_t prefix);
    b32 (*const endsWith)(cp_strview_t sv, cp_strview_t suffix);
    isize (*const find)(cp_strview_t sv, cp_strview_t needle);
    cp_strview_t *(*const split)(cp_strview_t sv, char delimiter, cp_arena_t *arena, usize *out_count);
    cp_string_t (*const format)(cp_arena_t *arena, const char *fmt, ...);
    cp_string_t (*const join)(cp_arena_t *arena, const cp_strview_t *views, usize count, cp_strview_t separator);
    cp_string_t (*const replace)(cp_arena_t *arena, cp_strview_t src, cp_strview_t target, cp_strview_t replacement);
} cp_str_namespace_t;

static const cp_str_namespace_t str = {
    .fromC = cp_sv_from_c,
    .sub = cp_sv_sub,
    .trim = cp_sv_trim,
    .equals = cp_sv_equals,
    .startsWith = cp_sv_starts_with,
    .endsWith = cp_sv_ends_with,
    .find = cp_sv_find,
    .split = cp_sv_split_arena,
    .format = cp_str_format,
    .join = cp_str_join,
    .replace = cp_str_replace,
};

typedef struct {
    i64 (*const encode)(const void *src, usize src_len, char *dst, usize dst_cap);
    i64 (*const decode)(const char *src, usize src_len, void *dst, usize dst_cap);
    cp_strview_t (*const encodeArena)(const void *src, usize src_len, cp_arena_t *arena);
    void *(*const decodeArena)(const char *src, usize src_len, cp_arena_t *arena, usize *out_len);
} cp_base64_namespace_t;

static const cp_base64_namespace_t base64 = {
    .encode = cp_base64_encode,
    .decode = cp_base64_decode,
    .encodeArena = cp_base64_encode_arena,
    .decodeArena = cp_base64_decode_arena,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_STR_H */

#ifdef CPLUS_CP_STR_IMPLEMENTATION
/**
 * @file cp_str.c
 * @brief Implementation of StringView operations and Arena-backed String Builder.
 */

#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* -------------------------------------------------------------------------
 * String View Implementation
 * ------------------------------------------------------------------------- */
cp_strview_t cp_sv_from_c(const char *cstr) {
    if (!cstr) return (cp_strview_t){ NULL, 0 };
    return (cp_strview_t){ cstr, strlen(cstr) };
}

cp_strview_t cp_sv_from_parts(const char *data, usize len) {
    return (cp_strview_t){ data, len };
}

cp_strview_t cp_sv_sub(cp_strview_t sv, usize start, usize len) {
    if (start >= sv.len) {
        return (cp_strview_t){ NULL, 0 };
    }
    if (start + len > sv.len) {
        len = sv.len - start;
    }
    return (cp_strview_t){ sv.data + start, len };
}

cp_strview_t cp_sv_trim_left(cp_strview_t sv) {
    while (sv.len > 0 && isspace((unsigned char)*sv.data)) {
        sv.data++;
        sv.len--;
    }
    return sv;
}

cp_strview_t cp_sv_trim_right(cp_strview_t sv) {
    while (sv.len > 0 && isspace((unsigned char)sv.data[sv.len - 1])) {
        sv.len--;
    }
    return sv;
}

cp_strview_t cp_sv_trim(cp_strview_t sv) {
    return cp_sv_trim_right(cp_sv_trim_left(sv));
}

b32 cp_sv_equals(cp_strview_t a, cp_strview_t b) {
    if (a.len != b.len) return false;
    if (a.len == 0) return true;
    return memcmp(a.data, b.data, a.len) == 0;
}

b32 cp_sv_equals_c(cp_strview_t a, const char *cstr) {
    if (!cstr) return a.len == 0;
    usize clen = strlen(cstr);
    if (a.len != clen) return false;
    return memcmp(a.data, cstr, clen) == 0;
}

b32 cp_sv_starts_with(cp_strview_t sv, cp_strview_t prefix) {
    if (prefix.len > sv.len) return false;
    return memcmp(sv.data, prefix.data, prefix.len) == 0;
}

b32 cp_sv_ends_with(cp_strview_t sv, cp_strview_t suffix) {
    if (suffix.len > sv.len) return false;
    return memcmp(sv.data + (sv.len - suffix.len), suffix.data, suffix.len) == 0;
}

isize cp_sv_find(cp_strview_t sv, cp_strview_t needle) {
    if (needle.len == 0) return 0;
    if (needle.len > sv.len) return -1;

    for (usize i = 0; i <= sv.len - needle.len; i++) {
        if (memcmp(sv.data + i, needle.data, needle.len) == 0) {
            return (isize)i;
        }
    }
    return -1;
}

isize cp_sv_find_char(cp_strview_t sv, char c) {
    for (usize i = 0; i < sv.len; i++) {
        if (sv.data[i] == c) {
            return (isize)i;
        }
    }
    return -1;
}

usize cp_sv_split(cp_strview_t sv, char delimiter, cp_strview_t *out_views, usize max_views) {
    if (max_views == 0 || !out_views) return 0;

    usize count = 0;
    usize start = 0;

    for (usize i = 0; i < sv.len; i++) {
        if (sv.data[i] == delimiter) {
            if (count < max_views) {
                out_views[count++] = cp_sv_from_parts(sv.data + start, i - start);
            }
            start = i + 1;
        }
    }

    if (count < max_views) {
        out_views[count++] = cp_sv_from_parts(sv.data + start, sv.len - start);
    }

    return count;
}

cp_strview_t *cp_sv_split_arena(cp_strview_t sv, char delimiter, cp_arena_t *arena, usize *out_count) {
    if (!arena) arena = Scratch();

    /* Count items */
    usize count = 1;
    for (usize i = 0; i < sv.len; i++) {
        if (sv.data[i] == delimiter) count++;
    }

    cp_strview_t *result = (cp_strview_t *)ArenaAlloc(arena, count * sizeof(cp_strview_t));
    usize populated = cp_sv_split(sv, delimiter, result, count);
    if (out_count) *out_count = populated;
    return result;
}

/* -------------------------------------------------------------------------
 * Dynamic String / String Builder Implementation
 * ------------------------------------------------------------------------- */
static void _cp_str_ensure_cap(cp_string_t *str, usize additional) {
    if (str->len + additional + 1 <= str->cap) return;

    usize new_cap = (str->cap * 2) + additional + 16;
    char *new_data = (char *)ArenaAlloc(str->arena, new_cap);
    if (str->len > 0 && str->data) {
        memcpy(new_data, str->data, str->len);
    }
    str->data = new_data;
    str->cap = new_cap;
}

cp_string_t cp_str_new(cp_arena_t *arena, usize initial_capacity) {
    if (!arena) arena = Scratch();
    if (initial_capacity < 16) initial_capacity = 16;

    cp_string_t s;
    s.arena = arena;
    s.len = 0;
    s.cap = initial_capacity;
    s.data = (char *)ArenaAlloc(arena, initial_capacity);
    s.data[0] = '\0';
    return s;
}

cp_string_t cp_str_from_sv(cp_arena_t *arena, cp_strview_t sv) {
    cp_string_t s = cp_str_new(arena, sv.len + 1);
    cp_str_append_sv(&s, sv);
    return s;
}

cp_string_t cp_str_from_c(cp_arena_t *arena, const char *cstr) {
    return cp_str_from_sv(arena, cp_sv_from_c(cstr));
}

void cp_str_append_char(cp_string_t *str, char c) {
    _cp_str_ensure_cap(str, 1);
    str->data[str->len++] = c;
    str->data[str->len] = '\0';
}

void cp_str_append_sv(cp_string_t *str, cp_strview_t sv) {
    if (sv.len == 0) return;
    _cp_str_ensure_cap(str, sv.len);
    memcpy(str->data + str->len, sv.data, sv.len);
    str->len += sv.len;
    str->data[str->len] = '\0';
}

void cp_str_append_c(cp_string_t *str, const char *cstr) {
    if (!cstr) return;
    cp_str_append_sv(str, cp_sv_from_c(cstr));
}

void cp_str_append_f(cp_string_t *str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (needed > 0) {
        _cp_str_ensure_cap(str, (usize)needed);
        vsnprintf(str->data + str->len, str->cap - str->len, fmt, args);
        str->len += (usize)needed;
    }
    va_end(args);
}

cp_string_t cp_str_format(cp_arena_t *arena, const char *fmt, ...) {
    if (!arena) arena = Scratch();

    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    cp_string_t s = cp_str_new(arena, (usize)needed + 1);
    if (needed > 0) {
        vsnprintf(s.data, s.cap, fmt, args);
        s.len = (usize)needed;
    }
    va_end(args);
    return s;
}

cp_string_t cp_str_format_scratch(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    cp_string_t s = cp_str_new(Scratch(), (usize)needed + 1);
    if (needed > 0) {
        vsnprintf(s.data, s.cap, fmt, args);
        s.len = (usize)needed;
    }
    va_end(args);
    return s;
}

cp_strview_t cp_str_to_sv(const cp_string_t *str) {
    if (!str || !str->data) return (cp_strview_t){ NULL, 0 };
    return (cp_strview_t){ str->data, str->len };
}

cp_string_t cp_str_join(cp_arena_t *arena, const cp_strview_t *views, usize count, cp_strview_t separator) {
    if (count == 0) return cp_str_new(arena, 16);

    usize total_len = 0;
    for (usize i = 0; i < count; i++) {
        total_len += views[i].len;
    }
    total_len += (count - 1) * separator.len;

    cp_string_t s = cp_str_new(arena, total_len + 1);
    for (usize i = 0; i < count; i++) {
        if (i > 0 && separator.len > 0) {
            cp_str_append_sv(&s, separator);
        }
        cp_str_append_sv(&s, views[i]);
    }
    return s;
}

cp_string_t cp_str_replace(cp_arena_t *arena, cp_strview_t src, cp_strview_t target, cp_strview_t replacement) {
    if (target.len == 0) return cp_str_from_sv(arena, src);

    cp_string_t s = cp_str_new(arena, src.len + 16);
    usize cursor = 0;

    while (cursor < src.len) {
        cp_strview_t remaining = cp_sv_sub(src, cursor, src.len - cursor);
        isize match_idx = cp_sv_find(remaining, target);
        if (match_idx == -1) {
            cp_str_append_sv(&s, remaining);
            break;
        }

        /* Append segment prior to match */
        cp_str_append_sv(&s, cp_sv_sub(remaining, 0, (usize)match_idx));
        /* Append replacement */
        cp_str_append_sv(&s, replacement);
        cursor += (usize)match_idx + target.len;
    }

    return s;
}

/* -------------------------------------------------------------------------
 * Base64 Encoding & Decoding Implementation
 * ------------------------------------------------------------------------- */
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const i8 b64_rev_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

i64 cp_base64_encode(const void *src, usize src_len, char *dst, usize dst_cap) {
    if (!src && src_len > 0) return -1;
    usize out_len = ((src_len + 2) / 3) * 4;
    if (!dst || dst_cap <= out_len) return (i64)out_len;

    const u8 *in = (const u8 *)src;
    usize i = 0, j = 0;
    while (i + 3 <= src_len) {
        u32 octet_a = in[i];
        u32 octet_b = in[i + 1];
        u32 octet_c = in[i + 2];
        i += 3;

        u32 triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        u8 c0 = (u8)b64_table[(triple >> 18) & 0x3F];
        u8 c1 = (u8)b64_table[(triple >> 12) & 0x3F];
        u8 c2 = (u8)b64_table[(triple >> 6) & 0x3F];
        u8 c3 = (u8)b64_table[triple & 0x3F];

        u32 word = (u32)c0 | ((u32)c1 << 8) | ((u32)c2 << 16) | ((u32)c3 << 24);
        memcpy(&dst[j], &word, 4);
        j += 4;
    }

    if (i < src_len) {
        usize rem = src_len - i;
        if (rem == 1) {
            u32 octet_a = in[i];
            u32 triple = (octet_a << 16);
            dst[j++] = b64_table[(triple >> 18) & 0x3F];
            dst[j++] = b64_table[(triple >> 12) & 0x3F];
            dst[j++] = '=';
            dst[j++] = '=';
        } else if (rem == 2) {
            u32 octet_a = in[i];
            u32 octet_b = in[i + 1];
            u32 triple = (octet_a << 16) | (octet_b << 8);
            dst[j++] = b64_table[(triple >> 18) & 0x3F];
            dst[j++] = b64_table[(triple >> 12) & 0x3F];
            dst[j++] = b64_table[(triple >> 6) & 0x3F];
            dst[j++] = '=';
        }
    }

    dst[j] = '\0';
    return (i64)j;
}

i64 cp_base64_decode(const char *src, usize src_len, void *dst, usize dst_cap) {
    if (!src && src_len > 0) return -1;
    if (src_len == 0) return 0;
    if (src_len % 4 != 0) return -1;

    usize pad = 0;
    if (src[src_len - 1] == '=') pad++;
    if (src_len >= 2 && src[src_len - 2] == '=') pad++;

    usize out_len = (src_len / 4) * 3 - pad;
    if (!dst || dst_cap < out_len) return (i64)out_len;

    u8 *out = (u8 *)dst;
    usize j = 0;
    for (usize i = 0; i < src_len; i += 4) {
        i8 b0 = b64_rev_table[(u8)src[i]];
        i8 b1 = b64_rev_table[(u8)src[i+1]];
        i8 b2 = (src[i+2] == '=') ? 0 : b64_rev_table[(u8)src[i+2]];
        i8 b3 = (src[i+3] == '=') ? 0 : b64_rev_table[(u8)src[i+3]];

        if (b0 < 0 || b1 < 0 || (src[i+2] != '=' && b2 < 0) || (src[i+3] != '=' && b3 < 0)) {
            return -1;
        }

        u32 triple = ((u32)b0 << 18) | ((u32)b1 << 12) | ((u32)b2 << 6) | (u32)b3;
        if (j < out_len) out[j++] = (triple >> 16) & 0xFF;
        if (j < out_len) out[j++] = (triple >> 8) & 0xFF;
        if (j < out_len) out[j++] = triple & 0xFF;
    }

    return (i64)j;
}

cp_strview_t cp_base64_encode_arena(const void *src, usize src_len, cp_arena_t *arena) {
    if (!arena) arena = Scratch();
    usize needed = ((src_len + 2) / 3) * 4;
    char *buf = (char *)ArenaAlloc(arena, needed + 1);
    i64 res = cp_base64_encode(src, src_len, buf, needed + 1);
    if (res < 0) return (cp_strview_t){ NULL, 0 };
    return (cp_strview_t){ buf, (usize)res };
}

void *cp_base64_decode_arena(const char *src, usize src_len, cp_arena_t *arena, usize *out_len) {
    if (!arena) arena = Scratch();
    if (src_len % 4 != 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    usize pad = 0;
    if (src_len > 0 && src[src_len - 1] == '=') pad++;
    if (src_len >= 2 && src[src_len - 2] == '=') pad++;
    usize max_len = (src_len / 4) * 3 - pad;

    u8 *buf = (u8 *)ArenaAlloc(arena, max_len > 0 ? max_len : 1);
    i64 res = cp_base64_decode(src, src_len, buf, max_len);
    if (res < 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    if (out_len) *out_len = (usize)res;
    return buf;
}
#endif /* CPLUS_CP_STR_IMPLEMENTATION */
