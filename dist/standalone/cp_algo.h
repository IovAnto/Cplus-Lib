/**
 * @file cp_algo.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_algo.
 *
 * In ONE file define:
 *   #define CPLUS_CP_ALGO_IMPLEMENTATION
 *   #include "cp_algo.h"
 */
#ifndef CPLUS_CP_ALGO_H
#define CPLUS_CP_ALGO_H

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
 * @file cp_algo.h
 * @brief High-performance algorithms: IntroSort, O(N) RadixSort, Binary Search, and Shuffling.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Sorting Algorithms
 * ------------------------------------------------------------------------- */
void cp_algo_sort_i32(i32 *arr, usize len);
void cp_algo_sort_u32(u32 *arr, usize len);
void cp_algo_sort_f32(f32 *arr, usize len);
void cp_algo_sort(void *base, usize num, usize size, int (*cmp)(const void *, const void *));

/**
 * @brief Ultra-fast O(N) LSD Radix Sort for 32-bit unsigned integers.
 */
void cp_algo_radix_sort_u32(u32 *arr, usize len);

/* -------------------------------------------------------------------------
 * Searching Algorithms
 * ------------------------------------------------------------------------- */
isize cp_algo_bsearch_i32(const i32 *arr, usize len, i32 key);
isize cp_algo_bsearch_u32(const u32 *arr, usize len, u32 key);

/* -------------------------------------------------------------------------
 * Utility Algorithms
 * ------------------------------------------------------------------------- */
void cp_algo_shuffle(void *base, usize num, usize size);
void cp_algo_reverse(void *base, usize num, usize size);

/* -------------------------------------------------------------------------
 * Fast Lock-Free Pseudo-Random Number Generator (Xoshiro256**)
 * ------------------------------------------------------------------------- */
typedef struct {
    u64 s[4];
} cp_rand_t;

typedef cp_rand_t Rng;

void cp_rand_init(cp_rand_t *rng, u64 seed);
u64 cp_rand_u64(cp_rand_t *rng);
f64 cp_rand_f64(cp_rand_t *rng);
u64 cp_rand_range(cp_rand_t *rng, u64 min, u64 max);

void cp_rand_seed(u64 seed);
u64 cp_rand_next_u64(void);
f64 cp_rand_next_f64(void);
u64 cp_rand_next_range(u64 min, u64 max);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define AlgoSortI32(arr, len)          cp_algo_sort_i32(arr, len)
#define AlgoSortU32(arr, len)          cp_algo_sort_u32(arr, len)
#define AlgoSortF32(arr, len)          cp_algo_sort_f32(arr, len)
#define AlgoSort(base, num, sz, cmp)   cp_algo_sort(base, num, sz, cmp)
#define AlgoRadixSortU32(arr, len)     cp_algo_radix_sort_u32(arr, len)

#define AlgoBinarySearchI32(a, l, k)   cp_algo_bsearch_i32(a, l, k)
#define AlgoBinarySearchU32(a, l, k)   cp_algo_bsearch_u32(a, l, k)

#define AlgoShuffle(base, num, size)   cp_algo_shuffle(base, num, size)
#define AlgoReverse(base, num, size)   cp_algo_reverse(base, num, size)

#define RandInit(rng, seed)            cp_rand_init(rng, seed)
#define RandU64(rng)                   cp_rand_u64(rng)
#define RandF64(rng)                   cp_rand_f64(rng)
#define RandRange(rng, min, max)       cp_rand_range(rng, min, max)
#define RandSeed(seed)                 cp_rand_seed(seed)
#define RandNextU64()                  cp_rand_next_u64()
#define RandNextF64()                  cp_rand_next_f64()
#define RandNextRange(min, max)        cp_rand_next_range(min, max)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*const sortI32)(i32 *arr, usize len);
    void (*const sortU32)(u32 *arr, usize len);
    void (*const sortF32)(f32 *arr, usize len);
    void (*const sort)(void *base, usize num, usize size, int (*cmp)(const void *, const void *));
    void (*const radixSortU32)(u32 *arr, usize len);
    isize (*const bsearchI32)(const i32 *arr, usize len, i32 key);
    isize (*const bsearchU32)(const u32 *arr, usize len, u32 key);
    void (*const shuffle)(void *base, usize num, usize size);
    void (*const reverse)(void *base, usize num, usize size);
} cp_algo_namespace_t;

static const cp_algo_namespace_t algo = {
    .sortI32 = cp_algo_sort_i32,
    .sortU32 = cp_algo_sort_u32,
    .sortF32 = cp_algo_sort_f32,
    .sort = cp_algo_sort,
    .radixSortU32 = cp_algo_radix_sort_u32,
    .bsearchI32 = cp_algo_bsearch_i32,
    .bsearchU32 = cp_algo_bsearch_u32,
    .shuffle = cp_algo_shuffle,
    .reverse = cp_algo_reverse,
};

typedef struct {
    void (*const init)(cp_rand_t *rng, u64 seed);
    u64 (*const u64_)(cp_rand_t *rng);
    f64 (*const f64_)(cp_rand_t *rng);
    u64 (*const range)(cp_rand_t *rng, u64 min, u64 max);
    void (*const seed)(u64 seed);
    u64 (*const nextU64)(void);
    f64 (*const nextF64)(void);
    u64 (*const nextRange)(u64 min, u64 max);
} cp_rng_namespace_t;

static const cp_rng_namespace_t rng = {
    .init = cp_rand_init,
    .u64_ = cp_rand_u64,
    .f64_ = cp_rand_f64,
    .range = cp_rand_range,
    .seed = cp_rand_seed,
    .nextU64 = cp_rand_next_u64,
    .nextF64 = cp_rand_next_f64,
    .nextRange = cp_rand_next_range,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_ALGO_H */

#ifdef CPLUS_CP_ALGO_IMPLEMENTATION
/**
 * @file cp_algo.c
 * @brief High-performance implementations of IntroSort, RadixSort, Binary Search, and Shuffling.
 */

#include <string.h>

#define INSERTION_SORT_THRESHOLD 16

/* -------------------------------------------------------------------------
 * Insertion Sort Helpers
 * ------------------------------------------------------------------------- */
static void _insertion_sort_i32(i32 *arr, usize len) {
    for (usize i = 1; i < len; i++) {
        i32 key = arr[i];
        isize j = (isize)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

static void _insertion_sort_u32(u32 *arr, usize len) {
    for (usize i = 1; i < len; i++) {
        u32 key = arr[i];
        isize j = (isize)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

static void _insertion_sort_f32(f32 *arr, usize len) {
    for (usize i = 1; i < len; i++) {
        f32 key = arr[i];
        isize j = (isize)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/* -------------------------------------------------------------------------
 * IntroSort / QuickSort Implementations
 * ------------------------------------------------------------------------- */
static void _quicksort_i32(i32 *arr, usize len) {
    while (len > INSERTION_SORT_THRESHOLD) {
        /* Median of three pivot */
        usize mid = len / 2;
        if (arr[0] > arr[mid]) { i32 t = arr[0]; arr[0] = arr[mid]; arr[mid] = t; }
        if (arr[0] > arr[len - 1]) { i32 t = arr[0]; arr[0] = arr[len - 1]; arr[len - 1] = t; }
        if (arr[mid] > arr[len - 1]) { i32 t = arr[mid]; arr[mid] = arr[len - 1]; arr[len - 1] = t; }

        i32 pivot = arr[mid];
        isize i = -1;
        isize j = (isize)len;

        for (;;) {
            do { i++; } while (arr[i] < pivot);
            do { j--; } while (arr[j] > pivot);
            if (i >= j) break;

            i32 tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }

        /* Tail-call recursion on smaller partition */
        usize left_len = (usize)(j + 1);
        usize right_len = len - left_len;

        if (left_len < right_len) {
            _quicksort_i32(arr, left_len);
            arr += left_len;
            len = right_len;
        } else {
            _quicksort_i32(arr + left_len, right_len);
            len = left_len;
        }
    }
    _insertion_sort_i32(arr, len);
}

void cp_algo_sort_i32(i32 *arr, usize len) {
    if (!arr || len < 2) return;
    _quicksort_i32(arr, len);
}

static void _quicksort_u32(u32 *arr, usize len) {
    while (len > INSERTION_SORT_THRESHOLD) {
        usize mid = len / 2;
        if (arr[0] > arr[mid]) { u32 t = arr[0]; arr[0] = arr[mid]; arr[mid] = t; }
        if (arr[0] > arr[len - 1]) { u32 t = arr[0]; arr[0] = arr[len - 1]; arr[len - 1] = t; }
        if (arr[mid] > arr[len - 1]) { u32 t = arr[mid]; arr[mid] = arr[len - 1]; arr[len - 1] = t; }

        u32 pivot = arr[mid];
        isize i = -1;
        isize j = (isize)len;

        for (;;) {
            do { i++; } while (arr[i] < pivot);
            do { j--; } while (arr[j] > pivot);
            if (i >= j) break;

            u32 tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }

        usize left_len = (usize)(j + 1);
        usize right_len = len - left_len;

        if (left_len < right_len) {
            _quicksort_u32(arr, left_len);
            arr += left_len;
            len = right_len;
        } else {
            _quicksort_u32(arr + left_len, right_len);
            len = left_len;
        }
    }
    _insertion_sort_u32(arr, len);
}

void cp_algo_sort_u32(u32 *arr, usize len) {
    if (!arr || len < 2) return;
    _quicksort_u32(arr, len);
}

static void _quicksort_f32(f32 *arr, usize len) {
    while (len > INSERTION_SORT_THRESHOLD) {
        usize mid = len / 2;
        if (arr[0] > arr[mid]) { f32 t = arr[0]; arr[0] = arr[mid]; arr[mid] = t; }
        if (arr[0] > arr[len - 1]) { f32 t = arr[0]; arr[0] = arr[len - 1]; arr[len - 1] = t; }
        if (arr[mid] > arr[len - 1]) { f32 t = arr[mid]; arr[mid] = arr[len - 1]; arr[len - 1] = t; }

        f32 pivot = arr[mid];
        isize i = -1;
        isize j = (isize)len;

        for (;;) {
            do { i++; } while (arr[i] < pivot);
            do { j--; } while (arr[j] > pivot);
            if (i >= j) break;

            f32 tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }

        usize left_len = (usize)(j + 1);
        usize right_len = len - left_len;

        if (left_len < right_len) {
            _quicksort_f32(arr, left_len);
            arr += left_len;
            len = right_len;
        } else {
            _quicksort_f32(arr + left_len, right_len);
            len = left_len;
        }
    }
    _insertion_sort_f32(arr, len);
}

void cp_algo_sort_f32(f32 *arr, usize len) {
    if (!arr || len < 2) return;
    _quicksort_f32(arr, len);
}

void cp_algo_sort(void *base, usize num, usize size, int (*cmp)(const void *, const void *)) {
    if (!base || num < 2 || !cmp) return;
    qsort(base, num, size, cmp);
}

/* -------------------------------------------------------------------------
 * O(N) LSD Radix Sort for u32
 * ------------------------------------------------------------------------- */
void cp_algo_radix_sort_u32(u32 *arr, usize len) {
    if (!arr || len < 2) return;

    cp_temp_arena_t scratch = ScratchBegin();
    u32 *temp = (u32 *)ArenaAlloc(scratch.arena, len * sizeof(u32));

    u32 *src = arr;
    u32 *dst = temp;

    /* 4 passes of 8 bits each (256 buckets) */
    for (usize shift = 0; shift < 32; shift += 8) {
        usize count[256] = {0};

        for (usize i = 0; i < len; i++) {
            u8 byte = (u8)((src[i] >> shift) & 0xFF);
            count[byte]++;
        }

        usize offset[256];
        offset[0] = 0;
        for (usize i = 1; i < 256; i++) {
            offset[i] = offset[i - 1] + count[i - 1];
        }

        for (usize i = 0; i < len; i++) {
            u8 byte = (u8)((src[i] >> shift) & 0xFF);
            dst[offset[byte]++] = src[i];
        }

        /* Swap src and dst pointers */
        u32 *swap = src;
        src = dst;
        dst = swap;
    }

    /* If final data is in temp, copy back to arr */
    if (src != arr) {
        memcpy(arr, src, len * sizeof(u32));
    }

    ScratchEnd(scratch);
}

/* -------------------------------------------------------------------------
 * Binary Search
 * ------------------------------------------------------------------------- */
isize cp_algo_bsearch_i32(const i32 *arr, usize len, i32 key) {
    if (!arr || len == 0) return -1;
    isize low = 0;
    isize high = (isize)len - 1;

    while (low <= high) {
        isize mid = low + (high - low) / 2;
        if (arr[mid] == key) return mid;
        if (arr[mid] < key) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

isize cp_algo_bsearch_u32(const u32 *arr, usize len, u32 key) {
    if (!arr || len == 0) return -1;
    isize low = 0;
    isize high = (isize)len - 1;

    while (low <= high) {
        isize mid = low + (high - low) / 2;
        if (arr[mid] == key) return mid;
        if (arr[mid] < key) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Utility Algorithms (Shuffle & Reverse)
 * ------------------------------------------------------------------------- */
static u32 _xorshift32_state = 2463534242U;

static u32 _xorshift32(void) {
    u32 x = _xorshift32_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    _xorshift32_state = x;
    return x;
}

void cp_algo_shuffle(void *base, usize num, usize size) {
    if (!base || num < 2 || size == 0) return;
    u8 *arr = (u8 *)base;
    u8 *temp = (u8 *)alloca(size);

    for (usize i = num - 1; i > 0; i--) {
        usize j = (usize)(_xorshift32() % (u32)(i + 1));
        if (i != j) {
            memcpy(temp, arr + i * size, size);
            memcpy(arr + i * size, arr + j * size, size);
            memcpy(arr + j * size, temp, size);
        }
    }
}

void cp_algo_reverse(void *base, usize num, usize size) {
    if (!base || num < 2 || size == 0) return;
    u8 *arr = (u8 *)base;
    u8 *temp = (u8 *)alloca(size);

    for (usize i = 0; i < num / 2; i++) {
        usize j = num - 1 - i;
        memcpy(temp, arr + i * size, size);
        memcpy(arr + i * size, arr + j * size, size);
        memcpy(arr + j * size, temp, size);
    }
}

/* -------------------------------------------------------------------------
 * Fast Lock-Free Pseudo-Random Number Generator (Xoshiro256**)
 * ------------------------------------------------------------------------- */
static inline u64 _xoshiro_rotl(const u64 x, int k) {
    return (x << k) | (x >> (64 - k));
}

static u64 _splitmix64(u64 *state) {
    u64 z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void cp_rand_init(cp_rand_t *rng, u64 seed) {
    if (!rng) return;
    u64 sm_state = (seed == 0) ? 0x853c49e6748fea9bULL : seed;
    rng->s[0] = _splitmix64(&sm_state);
    rng->s[1] = _splitmix64(&sm_state);
    rng->s[2] = _splitmix64(&sm_state);
    rng->s[3] = _splitmix64(&sm_state);
}

u64 cp_rand_u64(cp_rand_t *rng) {
    if (!rng) return 0;
    const u64 result = _xoshiro_rotl(rng->s[1] * 5, 7) * 9;
    const u64 t = rng->s[1] << 17;

    rng->s[2] ^= rng->s[0];
    rng->s[3] ^= rng->s[1];
    rng->s[1] ^= rng->s[2];
    rng->s[0] ^= rng->s[3];

    rng->s[2] ^= t;
    rng->s[3] = _xoshiro_rotl(rng->s[3], 45);

    return result;
}

f64 cp_rand_f64(cp_rand_t *rng) {
    return (f64)(cp_rand_u64(rng) >> 11) * (1.0 / 9007199254740992.0);
}

u64 cp_rand_range(cp_rand_t *rng, u64 min, u64 max) {
    if (min >= max) return min;
    u64 range = max - min + 1;
    return min + (cp_rand_u64(rng) % range);
}

static cp_rand_t _global_rng = {
    .s = { 0x123456789abcdef0ULL, 0xfedcba9876543210ULL, 0x0123456789abcdefULL, 0x89abcdef01234567ULL }
};

void cp_rand_seed(u64 seed) {
    cp_rand_init(&_global_rng, seed);
}

u64 cp_rand_next_u64(void) {
    return cp_rand_u64(&_global_rng);
}

f64 cp_rand_next_f64(void) {
    return cp_rand_f64(&_global_rng);
}

u64 cp_rand_next_range(u64 min, u64 max) {
    return cp_rand_range(&_global_rng, min, max);
}
#endif /* CPLUS_CP_ALGO_IMPLEMENTATION */
