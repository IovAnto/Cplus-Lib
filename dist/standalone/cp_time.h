/**
 * @file cp_time.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_time.
 *
 * In ONE file define:
 *   #define CPLUS_CP_TIME_IMPLEMENTATION
 *   #include "cp_time.h"
 */
#ifndef CPLUS_CP_TIME_H
#define CPLUS_CP_TIME_H

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
#include <time.h>

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
 * @file cp_time.h
 * @brief High-precision monotonic timers, stopwatch, ISO8601 timestamping, and RAII scoped profiling.
 */


#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Monotonic Time Primitives
 * ------------------------------------------------------------------------- */
u64 cp_time_now_ns(void);
f64 cp_time_now_us(void);
f64 cp_time_now_ms(void);
u64 cp_time_unix_sec(void);

void cp_time_sleep_ms(u32 ms);
void cp_time_sleep_us(u32 us);

void cp_time_iso8601(char *buf, usize cap);

/* -------------------------------------------------------------------------
 * Stopwatch
 * ------------------------------------------------------------------------- */
typedef struct {
    u64 start_ns;
    u64 stop_ns;
    bool running;
} cp_stopwatch_t;

cp_stopwatch_t cp_stopwatch_start(void);
void cp_stopwatch_stop(cp_stopwatch_t *sw);
void cp_stopwatch_reset(cp_stopwatch_t *sw);
f64 cp_stopwatch_elapsed_ns(const cp_stopwatch_t *sw);
f64 cp_stopwatch_elapsed_us(const cp_stopwatch_t *sw);
f64 cp_stopwatch_elapsed_ms(const cp_stopwatch_t *sw);
f64 cp_stopwatch_elapsed_sec(const cp_stopwatch_t *sw);

/* -------------------------------------------------------------------------
 * Scoped RAII Profiler
 * ------------------------------------------------------------------------- */
typedef struct {
    const char *name;
    u64 start_ns;
} _cp_prof_scope_t;

static inline void _cp_prof_scope_cleanup(_cp_prof_scope_t *scope) {
    if (scope && scope->name) {
        u64 diff = cp_time_now_ns() - scope->start_ns;
        printf("[PROFILE] \"%s\": %.3f ms (%.1f us)\n", scope->name, (f64)diff / 1000000.0, (f64)diff / 1000.0);
    }
}

#define ProfileScope(name_str) \
    _cp_prof_scope_t CP_CONCAT(_prof_scope_, __LINE__) __attribute__((cleanup(_cp_prof_scope_cleanup), unused)) = \
        { .name = (name_str), .start_ns = cp_time_now_ns() }

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define TimeNowNs()                      cp_time_now_ns()
#define TimeNowUs()                      cp_time_now_us()
#define TimeNowMs()                      cp_time_now_ms()
#define TimeUnixSec()                    cp_time_unix_sec()
#define TimeSleepMs(ms)                  cp_time_sleep_ms(ms)
#define TimeSleepUs(us)                  cp_time_sleep_us(us)
#define TimeIso8601(buf, cap)            cp_time_iso8601(buf, cap)

#define StopwatchStart()                 cp_stopwatch_start()
#define StopwatchStop(sw)                cp_stopwatch_stop(sw)
#define StopwatchReset(sw)               cp_stopwatch_reset(sw)
#define StopwatchElapsedNs(sw)           cp_stopwatch_elapsed_ns(sw)
#define StopwatchElapsedUs(sw)           cp_stopwatch_elapsed_us(sw)
#define StopwatchElapsedMs(sw)           cp_stopwatch_elapsed_ms(sw)
#define StopwatchElapsedSec(sw)          cp_stopwatch_elapsed_sec(sw)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    u64 (*const nowNs)(void);
    f64 (*const nowUs)(void);
    f64 (*const nowMs)(void);
    u64 (*const unixSec)(void);
    void (*const sleepMs)(u32 ms);
    void (*const sleepUs)(u32 us);
    void (*const iso8601)(char *buf, usize cap);
} cp_time_namespace_t;

typedef struct {
    cp_stopwatch_t (*const start)(void);
    void (*const stop)(cp_stopwatch_t *sw);
    void (*const reset)(cp_stopwatch_t *sw);
    f64 (*const elapsedNs)(const cp_stopwatch_t *sw);
    f64 (*const elapsedUs)(const cp_stopwatch_t *sw);
    f64 (*const elapsedMs)(const cp_stopwatch_t *sw);
    f64 (*const elapsedSec)(const cp_stopwatch_t *sw);
} cp_stopwatch_namespace_t;

static const cp_time_namespace_t time_api = {
    .nowNs = cp_time_now_ns,
    .nowUs = cp_time_now_us,
    .nowMs = cp_time_now_ms,
    .unixSec = cp_time_unix_sec,
    .sleepMs = cp_time_sleep_ms,
    .sleepUs = cp_time_sleep_us,
    .iso8601 = cp_time_iso8601,
};

static const cp_stopwatch_namespace_t stopwatch = {
    .start = cp_stopwatch_start,
    .stop = cp_stopwatch_stop,
    .reset = cp_stopwatch_reset,
    .elapsedNs = cp_stopwatch_elapsed_ns,
    .elapsedUs = cp_stopwatch_elapsed_us,
    .elapsedMs = cp_stopwatch_elapsed_ms,
    .elapsedSec = cp_stopwatch_elapsed_sec,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_TIME_H */

#ifdef CPLUS_CP_TIME_IMPLEMENTATION
/**
 * @file cp_time.c
 * @brief Implementation of monotonic time primitives, stopwatch, and ISO8601 formatting.
 */

#include <stdio.h>
#include <string.h>

u64 cp_time_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

f64 cp_time_now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec * 1000000.0 + (f64)ts.tv_nsec / 1000.0;
}

f64 cp_time_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
}

u64 cp_time_unix_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (u64)ts.tv_sec;
}

void cp_time_sleep_ms(u32 ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

void cp_time_sleep_us(u32 us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (long)(us % 1000000) * 1000L;
    nanosleep(&ts, NULL);
}

void cp_time_iso8601(char *buf, usize cap) {
    if (!buf || cap < 21) return;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_buf;
    gmtime_r(&ts.tv_sec, &tm_buf);
    strftime(buf, cap, "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
}

/* -------------------------------------------------------------------------
 * Stopwatch
 * ------------------------------------------------------------------------- */
cp_stopwatch_t cp_stopwatch_start(void) {
    cp_stopwatch_t sw;
    sw.start_ns = cp_time_now_ns();
    sw.stop_ns = 0;
    sw.running = true;
    return sw;
}

void cp_stopwatch_stop(cp_stopwatch_t *sw) {
    if (sw && sw->running) {
        sw->stop_ns = cp_time_now_ns();
        sw->running = false;
    }
}

void cp_stopwatch_reset(cp_stopwatch_t *sw) {
    if (sw) {
        sw->start_ns = cp_time_now_ns();
        sw->stop_ns = 0;
        sw->running = true;
    }
}

f64 cp_stopwatch_elapsed_ns(const cp_stopwatch_t *sw) {
    if (!sw) return 0.0;
    u64 end = sw->running ? cp_time_now_ns() : sw->stop_ns;
    return (f64)(end - sw->start_ns);
}

f64 cp_stopwatch_elapsed_us(const cp_stopwatch_t *sw) {
    return cp_stopwatch_elapsed_ns(sw) / 1000.0;
}

f64 cp_stopwatch_elapsed_ms(const cp_stopwatch_t *sw) {
    return cp_stopwatch_elapsed_ns(sw) / 1000000.0;
}

f64 cp_stopwatch_elapsed_sec(const cp_stopwatch_t *sw) {
    return cp_stopwatch_elapsed_ns(sw) / 1000000000.0;
}
#endif /* CPLUS_CP_TIME_IMPLEMENTATION */
