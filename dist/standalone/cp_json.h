/**
 * @file cp_json.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_json.
 *
 * In ONE file define:
 *   #define CPLUS_CP_JSON_IMPLEMENTATION
 *   #include "cp_json.h"
 */
#ifndef CPLUS_CP_JSON_H
#define CPLUS_CP_JSON_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <ctype.h>
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
 * @file cp_json.h
 * @brief Zero-allocation micro JSON parser and query engine operating over StrView slices.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * JSON Value Types & Structure
 * ------------------------------------------------------------------------- */
typedef enum {
    CP_JSON_INVALID = 0,
    CP_JSON_NULL,
    CP_JSON_BOOL,
    CP_JSON_NUMBER,
    CP_JSON_STRING,
    CP_JSON_ARRAY,
    CP_JSON_OBJECT
} cp_json_type_t;

typedef struct {
    StrView raw;
    cp_json_type_t type;
} cp_json_val_t;

/* -------------------------------------------------------------------------
 * Core JSON Operations
 * ------------------------------------------------------------------------- */
cp_json_val_t cp_json_parse(const char *json_str, usize len);
cp_json_val_t cp_json_get(cp_json_val_t obj, const char *key);
cp_json_val_t cp_json_at(cp_json_val_t arr, usize index);
usize cp_json_len(cp_json_val_t val);

StrView cp_json_as_view(cp_json_val_t val);
i64 cp_json_as_int(cp_json_val_t val, i64 default_val);
f64 cp_json_as_float(cp_json_val_t val, f64 default_val);
bool cp_json_as_bool(cp_json_val_t val, bool default_val);
bool cp_json_is_null(cp_json_val_t val);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define JsonParse(str, len)              cp_json_parse(str, len)
#define JsonGet(obj, key)                cp_json_get(obj, key)
#define JsonAt(arr, index)               cp_json_at(arr, index)
#define JsonLen(val)                     cp_json_len(val)
#define JsonAsView(val)                  cp_json_as_view(val)
#define JsonAsInt(val, def)              cp_json_as_int(val, def)
#define JsonAsFloat(val, def)            cp_json_as_float(val, def)
#define JsonAsBool(val, def)             cp_json_as_bool(val, def)
#define JsonIsNull(val)                  cp_json_is_null(val)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_json_val_t (*const parse)(const char *json_str, usize len);
    cp_json_val_t (*const get)(cp_json_val_t obj, const char *key);
    cp_json_val_t (*const at)(cp_json_val_t arr, usize index);
    usize (*const len)(cp_json_val_t val);
    StrView (*const asView)(cp_json_val_t val);
    i64 (*const asInt)(cp_json_val_t val, i64 default_val);
    f64 (*const asFloat)(cp_json_val_t val, f64 default_val);
    bool (*const asBool)(cp_json_val_t val, bool default_val);
    bool (*const isNull)(cp_json_val_t val);
} cp_json_namespace_t;

static const cp_json_namespace_t json_api = {
    .parse = cp_json_parse,
    .get = cp_json_get,
    .at = cp_json_at,
    .len = cp_json_len,
    .asView = cp_json_as_view,
    .asInt = cp_json_as_int,
    .asFloat = cp_json_as_float,
    .asBool = cp_json_as_bool,
    .isNull = cp_json_is_null,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_JSON_H */

#ifdef CPLUS_CP_JSON_IMPLEMENTATION
/**
 * @file cp_json.c
 * @brief Zero-allocation micro JSON parser implementation using StrView.
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static const char *skip_ws(const char *p, const char *end) {
    while (p < end && isspace((unsigned char)*p)) p++;
    return p;
}

static const char *skip_string(const char *p, const char *end) {
    if (p >= end || *p != '"') return p;
    p++;
    while (p < end) {
        if (*p == '\\') {
            p += 2;
        } else if (*p == '"') {
            return p + 1;
        } else {
            p++;
        }
    }
    return end;
}

static const char *skip_val(const char *p, const char *end) {
    p = skip_ws(p, end);
    if (p >= end) return end;

    if (*p == '"') {
        return skip_string(p, end);
    } else if (*p == '{') {
        p++;
        int depth = 1;
        while (p < end && depth > 0) {
            p = skip_ws(p, end);
            if (p >= end) break;
            if (*p == '"') {
                p = skip_string(p, end);
            } else {
                if (*p == '{') depth++;
                else if (*p == '}') depth--;
                p++;
            }
        }
        return p;
    } else if (*p == '[') {
        p++;
        int depth = 1;
        while (p < end && depth > 0) {
            p = skip_ws(p, end);
            if (p >= end) break;
            if (*p == '"') {
                p = skip_string(p, end);
            } else {
                if (*p == '[') depth++;
                else if (*p == ']') depth--;
                p++;
            }
        }
        return p;
    } else {
        /* Number, bool, or null: scan until delimiter */
        while (p < end && *p != ',' && *p != '}' && *p != ']' && !isspace((unsigned char)*p)) {
            p++;
        }
        return p;
    }
}

static cp_json_type_t detect_type(const char *p, const char *end) {
    p = skip_ws(p, end);
    if (p >= end) return CP_JSON_INVALID;
    if (*p == '{') return CP_JSON_OBJECT;
    if (*p == '[') return CP_JSON_ARRAY;
    if (*p == '"') return CP_JSON_STRING;
    if (*p == 't' || *p == 'f') return CP_JSON_BOOL;
    if (*p == 'n') return CP_JSON_NULL;
    if (*p == '-' || isdigit((unsigned char)*p)) return CP_JSON_NUMBER;
    return CP_JSON_INVALID;
}

cp_json_val_t cp_json_parse(const char *json_str, usize len) {
    cp_json_val_t v = { .raw = { NULL, 0 }, .type = CP_JSON_INVALID };
    if (!json_str) return v;
    if (len == 0) len = strlen(json_str);

    const char *start = skip_ws(json_str, json_str + len);
    const char *end = json_str + len;
    while (end > start && isspace((unsigned char)*(end - 1))) end--;

    if (start >= end) return v;

    v.raw = cp_sv_from_parts(start, (usize)(end - start));
    v.type = detect_type(start, end);
    return v;
}

cp_json_val_t cp_json_get(cp_json_val_t obj, const char *key) {
    cp_json_val_t res = { .raw = { NULL, 0 }, .type = CP_JSON_INVALID };
    if (obj.type != CP_JSON_OBJECT || !key || obj.raw.len < 2) return res;

    const char *p = obj.raw.data;
    const char *end = p + obj.raw.len;

    p = skip_ws(p, end);
    if (p >= end || *p != '{') return res;
    p++; /* skip '{' */

    usize key_len = strlen(key);

    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == '}') break;

        if (*p != '"') return res;
        const char *k_start = p + 1;
        const char *k_end = skip_string(p, end) - 1;
        p = k_end + 1;

        p = skip_ws(p, end);
        if (p >= end || *p != ':') return res;
        p++; /* skip ':' */

        p = skip_ws(p, end);
        const char *v_start = p;
        const char *v_end = skip_val(p, end);

        usize current_klen = (usize)(k_end - k_start);
        if (current_klen == key_len && memcmp(k_start, key, key_len) == 0) {
            res.raw = cp_sv_from_parts(v_start, (usize)(v_end - v_start));
            res.type = detect_type(v_start, v_end);
            return res;
        }

        p = skip_ws(v_end, end);
        if (p < end && *p == ',') {
            p++;
        }
    }
    return res;
}

cp_json_val_t cp_json_at(cp_json_val_t arr, usize index) {
    cp_json_val_t res = { .raw = { NULL, 0 }, .type = CP_JSON_INVALID };
    if (arr.type != CP_JSON_ARRAY || arr.raw.len < 2) return res;

    const char *p = arr.raw.data;
    const char *end = p + arr.raw.len;

    p = skip_ws(p, end);
    if (p >= end || *p != '[') return res;
    p++; /* skip '[' */

    usize current_idx = 0;

    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == ']') break;

        const char *v_start = p;
        const char *v_end = skip_val(p, end);

        if (current_idx == index) {
            res.raw = cp_sv_from_parts(v_start, (usize)(v_end - v_start));
            res.type = detect_type(v_start, v_end);
            return res;
        }

        current_idx++;
        p = skip_ws(v_end, end);
        if (p < end && *p == ',') {
            p++;
        }
    }
    return res;
}

usize cp_json_len(cp_json_val_t val) {
    if (val.type != CP_JSON_OBJECT && val.type != CP_JSON_ARRAY) return 0;
    if (val.raw.len < 2) return 0;

    const char *p = val.raw.data;
    const char *end = p + val.raw.len;
    p = skip_ws(p, end) + 1; /* skip { or [ */

    char close_char = (val.type == CP_JSON_OBJECT) ? '}' : ']';
    usize count = 0;

    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == close_char) break;

        if (val.type == CP_JSON_OBJECT) {
            p = skip_string(p, end);
            p = skip_ws(p, end);
            if (p < end && *p == ':') p++;
        }

        p = skip_val(p, end);
        count++;

        p = skip_ws(p, end);
        if (p < end && *p == ',') {
            p++;
        }
    }
    return count;
}

StrView cp_json_as_view(cp_json_val_t val) {
    if (val.type == CP_JSON_STRING && val.raw.len >= 2 && val.raw.data[0] == '"' && val.raw.data[val.raw.len - 1] == '"') {
        return cp_sv_from_parts(val.raw.data + 1, val.raw.len - 2);
    }
    return val.raw;
}

i64 cp_json_as_int(cp_json_val_t val, i64 default_val) {
    if (val.type != CP_JSON_NUMBER || val.raw.len == 0) return default_val;
    char buf[32];
    usize len = (val.raw.len < 31) ? val.raw.len : 31;
    memcpy(buf, val.raw.data, len);
    buf[len] = '\0';
    return (i64)strtoll(buf, NULL, 10);
}

f64 cp_json_as_float(cp_json_val_t val, f64 default_val) {
    if (val.type != CP_JSON_NUMBER || val.raw.len == 0) return default_val;
    char buf[64];
    usize len = (val.raw.len < 63) ? val.raw.len : 63;
    memcpy(buf, val.raw.data, len);
    buf[len] = '\0';
    return strtod(buf, NULL);
}

bool cp_json_as_bool(cp_json_val_t val, bool default_val) {
    if (val.type != CP_JSON_BOOL || val.raw.len == 0) return default_val;
    return (val.raw.len == 4 && memcmp(val.raw.data, "true", 4) == 0);
}

bool cp_json_is_null(cp_json_val_t val) {
    return val.type == CP_JSON_NULL;
}
#endif /* CPLUS_CP_JSON_IMPLEMENTATION */
