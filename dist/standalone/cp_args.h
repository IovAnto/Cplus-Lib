/**
 * @file cp_args.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_args.
 *
 * In ONE file define:
 *   #define CPLUS_CP_ARGS_IMPLEMENTATION
 *   #include "cp_args.h"
 */
#ifndef CPLUS_CP_ARGS_H
#define CPLUS_CP_ARGS_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
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
 * @file cp_args.h
 * @brief Modern, zero-allocation CLI argument parser supporting flags, options, and positionals.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Argument Parser Structures
 * ------------------------------------------------------------------------- */
#define CP_ARGS_MAX_POSITIONALS 64
#define CP_ARGS_MAX_OPTIONS     64
#define CP_ARGS_MAX_FLAGS       64

typedef struct {
    const char *key;
    const char *value;
} cp_arg_kv_t;

typedef struct {
    int argc;
    char **argv;
    const char *program_name;
    const char *positionals[CP_ARGS_MAX_POSITIONALS];
    usize positional_count;
    cp_arg_kv_t options[CP_ARGS_MAX_OPTIONS];
    usize option_count;
    const char *flags[CP_ARGS_MAX_FLAGS];
    usize flag_count;
} cp_args_t;

/* -------------------------------------------------------------------------
 * Parser Operations
 * ------------------------------------------------------------------------- */
cp_args_t cp_args_parse(int argc, char **argv);
bool cp_args_has_flag(const cp_args_t *args, const char *short_flag, const char *long_flag);
const char *cp_args_get_str(const cp_args_t *args, const char *short_opt, const char *long_opt, const char *default_val);
i64 cp_args_get_int(const cp_args_t *args, const char *short_opt, const char *long_opt, i64 default_val);
f64 cp_args_get_float(const cp_args_t *args, const char *short_opt, const char *long_opt, f64 default_val);
const char *cp_args_get_positional(const cp_args_t *args, usize index);
usize cp_args_positional_count(const cp_args_t *args);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define ArgsParse(argc, argv)            cp_args_parse(argc, argv)
#define ArgsHasFlag(a, sf, lf)           cp_args_has_flag(a, sf, lf)
#define ArgsGetStr(a, sf, lf, def)       cp_args_get_str(a, sf, lf, def)
#define ArgsGetInt(a, sf, lf, def)       cp_args_get_int(a, sf, lf, def)
#define ArgsGetFloat(a, sf, lf, def)     cp_args_get_float(a, sf, lf, def)
#define ArgsGetPositional(a, idx)        cp_args_get_positional(a, idx)
#define ArgsPositionalCount(a)           cp_args_positional_count(a)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_args_t (*const parse)(int argc, char **argv);
    bool (*const hasFlag)(const cp_args_t *args, const char *short_flag, const char *long_flag);
    const char *(*const getStr)(const cp_args_t *args, const char *short_opt, const char *long_opt, const char *default_val);
    i64 (*const getInt)(const cp_args_t *args, const char *short_opt, const char *long_opt, i64 default_val);
    f64 (*const getFloat)(const cp_args_t *args, const char *short_opt, const char *long_opt, f64 default_val);
    const char *(*const getPositional)(const cp_args_t *args, usize index);
    usize (*const positionalCount)(const cp_args_t *args);
} cp_args_namespace_t;

static const cp_args_namespace_t args = {
    .parse = cp_args_parse,
    .hasFlag = cp_args_has_flag,
    .getStr = cp_args_get_str,
    .getInt = cp_args_get_int,
    .getFloat = cp_args_get_float,
    .getPositional = cp_args_get_positional,
    .positionalCount = cp_args_positional_count,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_ARGS_H */

#ifdef CPLUS_CP_ARGS_IMPLEMENTATION
/**
 * @file cp_args.c
 * @brief Implementation of modern, zero-allocation CLI argument parser.
 */

#include <string.h>
#include <stdlib.h>

cp_args_t cp_args_parse(int argc, char **argv) {
    cp_args_t res;
    memset(&res, 0, sizeof(res));
    res.argc = argc;
    res.argv = argv;
    if (argc > 0 && argv) {
        res.program_name = argv[0];
    }

    bool parse_opts = true;
    int i = 1;

    while (i < argc) {
        const char *arg = argv[i];
        if (!arg) {
            i++;
            continue;
        }

        if (parse_opts && strcmp(arg, "--") == 0) {
            parse_opts = false;
            i++;
            continue;
        }

        if (parse_opts && arg[0] == '-' && arg[1] != '\0') {
            /* Record as flag */
            if (res.flag_count < CP_ARGS_MAX_FLAGS) {
                res.flags[res.flag_count++] = arg;
            }

            const char *eq = strchr(arg, '=');
            if (eq) {
                /* Key-Value with '=': e.g. --port=8080 */
                if (res.option_count < CP_ARGS_MAX_OPTIONS) {
                    res.options[res.option_count].key = arg;
                    res.options[res.option_count].value = eq + 1;
                    res.option_count++;
                }
            } else if (i + 1 < argc && argv[i + 1] && argv[i + 1][0] != '-') {
                /* Key followed by value: e.g. --port 8080 */
                if (res.option_count < CP_ARGS_MAX_OPTIONS) {
                    res.options[res.option_count].key = arg;
                    res.options[res.option_count].value = argv[i + 1];
                    res.option_count++;
                }
                /* Also preserve argv[i+1] in positionals in case arg was a boolean flag */
                if (res.positional_count < CP_ARGS_MAX_POSITIONALS) {
                    res.positionals[res.positional_count++] = argv[i + 1];
                }
                i++; /* consumed value */
            }
        } else {
            /* Positional argument */
            if (res.positional_count < CP_ARGS_MAX_POSITIONALS) {
                res.positionals[res.positional_count++] = arg;
            }
        }
        i++;
    }
    return res;
}

bool cp_args_has_flag(const cp_args_t *args, const char *short_flag, const char *long_flag) {
    if (!args) return false;
    for (usize i = 0; i < args->flag_count; ++i) {
        const char *f = args->flags[i];
        const char *eq = strchr(f, '=');
        usize klen = eq ? (usize)(eq - f) : strlen(f);

        if (short_flag && strncmp(f, short_flag, klen) == 0 && strlen(short_flag) == klen) {
            return true;
        }
        if (long_flag && strncmp(f, long_flag, klen) == 0 && strlen(long_flag) == klen) {
            return true;
        }
    }
    return false;
}

const char *cp_args_get_str(const cp_args_t *args, const char *short_opt, const char *long_opt, const char *default_val) {
    if (!args) return default_val;
    for (usize i = 0; i < args->option_count; ++i) {
        const char *k = args->options[i].key;
        const char *eq = strchr(k, '=');
        usize klen = eq ? (usize)(eq - k) : strlen(k);

        if (short_opt && strncmp(k, short_opt, klen) == 0 && strlen(short_opt) == klen) {
            return args->options[i].value;
        }
        if (long_opt && strncmp(k, long_opt, klen) == 0 && strlen(long_opt) == klen) {
            return args->options[i].value;
        }
    }
    return default_val;
}

i64 cp_args_get_int(const cp_args_t *args, const char *short_opt, const char *long_opt, i64 default_val) {
    const char *s = cp_args_get_str(args, short_opt, long_opt, NULL);
    if (!s) return default_val;
    return (i64)strtoll(s, NULL, 0);
}

f64 cp_args_get_float(const cp_args_t *args, const char *short_opt, const char *long_opt, f64 default_val) {
    const char *s = cp_args_get_str(args, short_opt, long_opt, NULL);
    if (!s) return default_val;
    return strtod(s, NULL);
}

const char *cp_args_get_positional(const cp_args_t *args, usize index) {
    if (!args || index >= args->positional_count) return NULL;
    return args->positionals[index];
}

usize cp_args_positional_count(const cp_args_t *args) {
    return args ? args->positional_count : 0;
}
#endif /* CPLUS_CP_ARGS_IMPLEMENTATION */
