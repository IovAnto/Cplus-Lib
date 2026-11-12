/**
 * @file cplus.h
 * @brief Cplus Amalgamated Single-Header Library
 * @version 0.1.0
 * @author Cplus Systems Architecture
 *
 * Modern Zero-Latency Foundation Library for C (C11).
 * Dual PascalCase and Dot-Notation API.
 *
 * USAGE:
 *   In exactly ONE compilation unit (.c file), add:
 *     #define CPLUS_IMPLEMENTATION
 *     #include "cplus.h"
 *
 *   In all other files:
 *     #include "cplus.h"
 */

#ifndef CPLUS_AMALGAMATED_H
#define CPLUS_AMALGAMATED_H

/* Platform & Standard Library Includes */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <alloca.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <immintrin.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <math.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * HEADER DECLARATIONS & INLINED TYPES
 * ========================================================================= */

/* --- Module: cp_core --- */
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

/* --- Module: cp_mem --- */
/**
 * @file cp_mem.h
 * @brief Zero-latency memory architecture: Arena allocator, Scratch pools, 
 *        Fixed-size Pool allocator, and Tracked Heap memory.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Arena Allocator
 * ------------------------------------------------------------------------- */
typedef struct cp_arena_chunk {
    struct cp_arena_chunk *next;
    usize capacity;
    usize offset;
    u8 data[];
} cp_arena_chunk_t;

typedef struct cp_arena {
    cp_arena_chunk_t *first;
    cp_arena_chunk_t *current;
    usize default_chunk_size;
    usize total_allocated;
} cp_arena_t;

typedef cp_arena_t Arena;

typedef struct {
    cp_arena_t *arena;
    cp_arena_chunk_t *chunk;
    usize offset;
} cp_temp_arena_t;

typedef cp_temp_arena_t TempArena;

cp_arena_t *cp_arena_create(usize default_chunk_size);
void cp_arena_destroy(cp_arena_t *arena);
void *cp_arena_alloc_align(cp_arena_t *arena, usize size, usize alignment);
void *cp_arena_alloc(cp_arena_t *arena, usize size);
void *cp_arena_alloc_zero(cp_arena_t *arena, usize size);
void cp_arena_reset(cp_arena_t *arena);

cp_temp_arena_t cp_temp_begin(cp_arena_t *arena);
void cp_temp_end(cp_temp_arena_t temp);

/* -------------------------------------------------------------------------
 * Thread-Local Scratch Arena
 * ------------------------------------------------------------------------- */
cp_arena_t *cp_scratch_get(void);
cp_temp_arena_t cp_scratch_begin(void);
void cp_scratch_end(cp_temp_arena_t temp);

/* -------------------------------------------------------------------------
 * Fixed-Size Pool Allocator
 * ------------------------------------------------------------------------- */
typedef struct cp_pool_node {
    struct cp_pool_node *next;
} cp_pool_node_t;

typedef struct cp_pool {
    u8 *buffer;
    cp_pool_node_t *free_list;
    usize item_size;
    usize item_count;
    usize capacity;
    usize used;
} cp_pool_t;

typedef cp_pool_t Pool;

cp_pool_t *cp_pool_create(usize item_size, usize item_count);
void cp_pool_destroy(cp_pool_t *pool);
void *cp_pool_alloc(cp_pool_t *pool);
void cp_pool_free(cp_pool_t *pool, void *ptr);

/* -------------------------------------------------------------------------
 * Tracked Heap Allocator (Debug Safety & Leak Reporting)
 * ------------------------------------------------------------------------- */
void *cp_heap_alloc_debug(usize size, const char *file, int line);
void *cp_heap_realloc_debug(void *ptr, usize size, const char *file, int line);
void cp_heap_free_debug(void *ptr, const char *file, int line);
usize cp_heap_active_leaks(void);
void cp_heap_report_leaks(void);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define ArenaCreate(chunk_size)        cp_arena_create(chunk_size)
#define ArenaDestroy(arena)            cp_arena_destroy(arena)
#define ArenaAlloc(arena, size)        cp_arena_alloc(arena, size)
#define ArenaAllocZero(arena, size)    cp_arena_alloc_zero(arena, size)
#define ArenaAllocAlign(a, sz, align)  cp_arena_alloc_align(a, sz, align)
#define ArenaReset(arena)              cp_arena_reset(arena)

#define TempBegin(arena)               cp_temp_begin(arena)
#define TempEnd(temp)                  cp_temp_end(temp)

#define Scratch()                      cp_scratch_get()
#define ScratchBegin()                 cp_scratch_begin()
#define ScratchEnd(temp)               cp_scratch_end(temp)

#define PoolCreate(item_sz, count)     cp_pool_create(item_sz, count)
#define PoolDestroy(pool)              cp_pool_destroy(pool)
#define PoolAlloc(pool)                cp_pool_alloc(pool)
#define PoolFree(pool, ptr)            cp_pool_free(pool, ptr)

#define MemAlloc(size)                 cp_heap_alloc_debug(size, __FILE__, __LINE__)
#define MemRealloc(ptr, size)          cp_heap_realloc_debug(ptr, size, __FILE__, __LINE__)
#define MemFree(ptr)                   cp_heap_free_debug(ptr, __FILE__, __LINE__)
#define MemReportLeaks()               cp_heap_report_leaks()
#define MemActiveLeaks()               cp_heap_active_leaks()

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace Structs
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_arena_t *(*const create)(usize default_chunk_size);
    void (*const destroy)(cp_arena_t *arena);
    void *(*const alloc)(cp_arena_t *arena, usize size);
    void *(*const allocZero)(cp_arena_t *arena, usize size);
    void *(*const allocAlign)(cp_arena_t *arena, usize size, usize alignment);
    void (*const reset)(cp_arena_t *arena);
    cp_temp_arena_t (*const tempBegin)(cp_arena_t *arena);
    void (*const tempEnd)(cp_temp_arena_t temp);
} cp_arena_namespace_t;

typedef struct {
    cp_pool_t *(*const create)(usize item_size, usize item_count);
    void (*const destroy)(cp_pool_t *pool);
    void *(*const alloc)(cp_pool_t *pool);
    void (*const free)(cp_pool_t *pool, void *ptr);
} cp_pool_namespace_t;

typedef struct {
    void *(*const alloc)(usize size);
    void *(*const realloc)(void *ptr, usize size);
    void (*const free)(void *ptr);
    usize (*const activeLeaks)(void);
    void (*const reportLeaks)(void);
    cp_arena_t *(*const scratch)(void);
} cp_mem_namespace_t;

CP_INLINE void *_cp_ns_heap_alloc(usize size) {
    return cp_heap_alloc_debug(size, __FILE__, __LINE__);
}
CP_INLINE void *_cp_ns_heap_realloc(void *ptr, usize size) {
    return cp_heap_realloc_debug(ptr, size, __FILE__, __LINE__);
}
CP_INLINE void _cp_ns_heap_free(void *ptr) {
    cp_heap_free_debug(ptr, __FILE__, __LINE__);
}

static const cp_arena_namespace_t arena = {
    .create = cp_arena_create,
    .destroy = cp_arena_destroy,
    .alloc = cp_arena_alloc,
    .allocZero = cp_arena_alloc_zero,
    .allocAlign = cp_arena_alloc_align,
    .reset = cp_arena_reset,
    .tempBegin = cp_temp_begin,
    .tempEnd = cp_temp_end,
};

static const cp_pool_namespace_t pool = {
    .create = cp_pool_create,
    .destroy = cp_pool_destroy,
    .alloc = cp_pool_alloc,
    .free = cp_pool_free,
};

static const cp_mem_namespace_t mem = {
    .alloc = _cp_ns_heap_alloc,
    .realloc = _cp_ns_heap_realloc,
    .free = _cp_ns_heap_free,
    .activeLeaks = cp_heap_active_leaks,
    .reportLeaks = cp_heap_report_leaks,
    .scratch = cp_scratch_get,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_str --- */
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

/* --- Module: cp_fs --- */
/**
 * @file cp_fs.h
 * @brief High-level, zero-latency Filesystem and Path operations.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Filesystem Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_fs_read(cp_arena_t *arena, const char *path);
b32 cp_fs_write(const char *path, const void *data, usize len);
b32 cp_fs_append(const char *path, const void *data, usize len);

b32 cp_fs_exists(const char *path);
b32 cp_fs_is_dir(const char *path);
usize cp_fs_size(const char *path);
b32 cp_fs_remove(const char *path);
b32 cp_fs_mkdir(const char *path);

cp_strview_t *cp_fs_list_dir(cp_arena_t *arena, const char *path, usize *out_count);

/* -------------------------------------------------------------------------
 * Path Manipulation Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_path_join(cp_arena_t *arena, cp_strview_t a, cp_strview_t b);
cp_strview_t cp_path_ext(cp_strview_t path);
cp_strview_t cp_path_base(cp_strview_t path);
cp_strview_t cp_path_dir(cp_arena_t *arena, cp_strview_t path);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define FileRead(arena, path)          cp_fs_read(arena, path)
#define FileReadScratch(path)          cp_fs_read(Scratch(), path)
#define FileWrite(path, data, len)     cp_fs_write(path, data, len)
#define FileWriteStr(path, strview)    cp_fs_write(path, (strview).data, (strview).len)
#define FileAppend(path, data, len)    cp_fs_append(path, data, len)
#define FileAppendStr(path, strview)   cp_fs_append(path, (strview).data, (strview).len)

#define FileExists(path)               cp_fs_exists(path)
#define FileIsDir(path)                cp_fs_is_dir(path)
#define FileSize(path)                 cp_fs_size(path)
#define FileRemove(path)               cp_fs_remove(path)
#define FileMkdir(path)                cp_fs_mkdir(path)
#define FileListDir(arena, path, count) cp_fs_list_dir(arena, path, count)

#define PathJoin(arena, a, b)          cp_path_join(arena, a, b)
#define PathExt(path)                  cp_path_ext(path)
#define PathBase(path)                 cp_path_base(path)
#define PathDir(arena, path)           cp_path_dir(arena, path)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_strview_t (*const read)(cp_arena_t *arena, const char *path);
    b32 (*const write)(const char *path, const void *data, usize len);
    b32 (*const append)(const char *path, const void *data, usize len);
    b32 (*const exists)(const char *path);
    b32 (*const isDir)(const char *path);
    usize (*const size)(const char *path);
    b32 (*const remove)(const char *path);
    b32 (*const mkdir)(const char *path);
    cp_strview_t *(*const listDir)(cp_arena_t *arena, const char *path, usize *out_count);
} cp_fs_namespace_t;

typedef struct {
    cp_strview_t (*const join)(cp_arena_t *arena, cp_strview_t a, cp_strview_t b);
    cp_strview_t (*const ext)(cp_strview_t path);
    cp_strview_t (*const base)(cp_strview_t path);
    cp_strview_t (*const dir)(cp_arena_t *arena, cp_strview_t path);
} cp_path_namespace_t;

static const cp_fs_namespace_t fs = {
    .read = cp_fs_read,
    .write = cp_fs_write,
    .append = cp_fs_append,
    .exists = cp_fs_exists,
    .isDir = cp_fs_is_dir,
    .size = cp_fs_size,
    .remove = cp_fs_remove,
    .mkdir = cp_fs_mkdir,
    .listDir = cp_fs_list_dir,
};

static const cp_path_namespace_t path = {
    .join = cp_path_join,
    .ext = cp_path_ext,
    .base = cp_path_base,
    .dir = cp_path_dir,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_ds --- */
/**
 * @file cp_ds.h
 * @brief Zero-latency data structures: Type-safe Vector, Singly Linked List,
 *        Doubly Linked List, and SPSC Lock-Free RingBuffer.
 */


#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Dynamic Type-Safe Vector (Contiguous Layout, Stretchy Buffer Pattern)
 * ------------------------------------------------------------------------- */
typedef struct {
    usize len;
    usize cap;
} cp_vec_header_t;

#define cp_vec_header(v)        ((cp_vec_header_t *)(v) - 1)
#define cp_vec_len(v)           ((v) ? cp_vec_header(v)->len : 0)
#define cp_vec_cap(v)           ((v) ? cp_vec_header(v)->cap : 0)

void *_cp_vec_grow(void *vec, usize elem_size, usize add_len, usize min_cap);
void _cp_vec_free(void *vec);

#define Vec(T)                  T*
#define VecNew(T)               ((T *)NULL)
#define VecLen(v)               cp_vec_len(v)
#define VecCap(v)               cp_vec_cap(v)

#define VecPush(v, val) do { \
    if (!((v) && cp_vec_header(v)->len < cp_vec_header(v)->cap)) { \
        (v) = _cp_vec_grow((v), sizeof(*(v)), 1, 4); \
    } \
    (v)[cp_vec_header(v)->len++] = (val); \
} while(0)

#define VecPop(v) do { \
    if ((v) && cp_vec_header(v)->len > 0) { \
        cp_vec_header(v)->len--; \
    } \
} while(0)

#define VecClear(v) do { \
    if (v) { cp_vec_header(v)->len = 0; } \
} while(0)

#define VecFree(v) do { \
    if (v) { _cp_vec_free(v); (v) = NULL; } \
} while(0)

/* Functional Iteration Macros (Zero Overhead Inlined Loops) */
#define VecEach(v, idx, val, body) do { \
    usize _v_len = VecLen(v); \
    for (usize idx = 0; idx < _v_len; idx++) { \
        __typeof__(*(v)) val = (v)[idx]; \
        CP_UNUSED(val); \
        body; \
    } \
} while(0)

#define VecMap(v, item_ptr, body) do { \
    usize _v_len = VecLen(v); \
    for (usize _i = 0; _i < _v_len; _i++) { \
        __typeof__(v) item_ptr = &(v)[_i]; \
        CP_UNUSED(item_ptr); \
        body; \
    } \
} while(0)

/* -------------------------------------------------------------------------
 * Singly Linked List (Node & List)
 * ------------------------------------------------------------------------- */
typedef struct cp_snode {
    struct cp_snode *next;
    void *val;
} cp_snode_t;

typedef struct cp_slist {
    cp_snode_t *head;
    cp_snode_t *tail;
    usize len;
    cp_arena_t *arena;
} cp_slist_t;

typedef cp_slist_t SList;

cp_slist_t cp_slist_new(cp_arena_t *arena);
void cp_slist_push_front(cp_slist_t *list, void *val);
void cp_slist_push_back(cp_slist_t *list, void *val);
void *cp_slist_pop_front(cp_slist_t *list);
void cp_slist_clear(cp_slist_t *list);

#define ListNew(arena)             cp_slist_new(arena)
#define ListPushFront(list, val)   cp_slist_push_front(list, (void *)(val))
#define ListPushBack(list, val)    cp_slist_push_back(list, (void *)(val))
#define ListPopFront(list)         cp_slist_pop_front(list)
#define ListLen(list)              ((list)->len)

/* -------------------------------------------------------------------------
 * Doubly Linked List (Node & List with O(1) arbitrary removal)
 * ------------------------------------------------------------------------- */
typedef struct cp_dnode {
    struct cp_dnode *prev;
    struct cp_dnode *next;
    void *val;
} cp_dnode_t;

typedef struct cp_dlist {
    cp_dnode_t *head;
    cp_dnode_t *tail;
    usize len;
    cp_arena_t *arena;
} cp_dlist_t;

typedef cp_dlist_t DList;

cp_dlist_t cp_dlist_new(cp_arena_t *arena);
cp_dnode_t *cp_dlist_push_front(cp_dlist_t *list, void *val);
cp_dnode_t *cp_dlist_push_back(cp_dlist_t *list, void *val);
void *cp_dlist_pop_front(cp_dlist_t *list);
void *cp_dlist_pop_back(cp_dlist_t *list);
void cp_dlist_remove(cp_dlist_t *list, cp_dnode_t *node);
cp_dnode_t *cp_dlist_insert_after(cp_dlist_t *list, cp_dnode_t *node, void *val);
void cp_dlist_clear(cp_dlist_t *list);

#define DListNew(arena)                 cp_dlist_new(arena)
#define DListPushFront(list, val)       cp_dlist_push_front(list, (void *)(val))
#define DListPushBack(list, val)        cp_dlist_push_back(list, (void *)(val))
#define DListPopFront(list)             cp_dlist_pop_front(list)
#define DListPopBack(list)              cp_dlist_pop_back(list)
#define DListRemove(list, node)         cp_dlist_remove(list, node)
#define DListInsertAfter(list, node, v) cp_dlist_insert_after(list, node, (void *)(v))
#define DListLen(list)                  ((list)->len)

/* -------------------------------------------------------------------------
 * Lock-Free SPSC RingBuffer (Single Producer, Single Consumer)
 * Cache-line padded (64 bytes) to prevent false sharing between CPU cores.
 * ------------------------------------------------------------------------- */
typedef struct cp_ringbuf {
    void **buffer;
    usize capacity;
    usize mask;

    /* Write index (producer owned) with cache-line padding */
    CP_CACHELINE _Atomic usize tail;
    char _pad1[CP_CACHELINE_SIZE - sizeof(_Atomic usize)];

    /* Read index (consumer owned) with cache-line padding */
    CP_CACHELINE _Atomic usize head;
    char _pad2[CP_CACHELINE_SIZE - sizeof(_Atomic usize)];
} cp_ringbuf_t;

typedef cp_ringbuf_t RingBuf;
typedef cp_ringbuf_t RingBuffer;

cp_ringbuf_t *cp_ringbuf_create(usize capacity_power_of_two);
void cp_ringbuf_destroy(cp_ringbuf_t *rb);
b32 cp_ringbuf_push(cp_ringbuf_t *rb, void *item);
b32 cp_ringbuf_pop(cp_ringbuf_t *rb, void **out_item);
usize cp_ringbuf_count(const cp_ringbuf_t *rb);

#define RingBufCreate(cap)         cp_ringbuf_create(cap)
#define RingBufDestroy(rb)         cp_ringbuf_destroy(rb)
#define RingBufPush(rb, item)      cp_ringbuf_push(rb, (void *)(item))
#define RingBufPop(rb, out_ptr)    cp_ringbuf_pop(rb, (void **)(out_ptr))
#define RingBufCount(rb)           cp_ringbuf_count(rb)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespaces
 * ------------------------------------------------------------------------- */
typedef struct {
    usize (*const len)(const void *vec);
    usize (*const cap)(const void *vec);
    void (*const free)(void *vec);
} cp_vec_namespace_t;

typedef struct {
    cp_slist_t (*const create)(cp_arena_t *arena);
    void (*const pushFront)(cp_slist_t *list, void *val);
    void (*const pushBack)(cp_slist_t *list, void *val);
    void *(*const popFront)(cp_slist_t *list);
    usize (*const len)(const cp_slist_t *list);
    void (*const clear)(cp_slist_t *list);
} cp_slist_namespace_t;

typedef struct {
    cp_dlist_t (*const create)(cp_arena_t *arena);
    cp_dnode_t *(*const pushFront)(cp_dlist_t *list, void *val);
    cp_dnode_t *(*const pushBack)(cp_dlist_t *list, void *val);
    void *(*const popFront)(cp_dlist_t *list);
    void *(*const popBack)(cp_dlist_t *list);
    void (*const remove)(cp_dlist_t *list, cp_dnode_t *node);
    cp_dnode_t *(*const insertAfter)(cp_dlist_t *list, cp_dnode_t *node, void *val);
    usize (*const len)(const cp_dlist_t *list);
    void (*const clear)(cp_dlist_t *list);
} cp_dlist_namespace_t;

typedef struct {
    cp_ringbuf_t *(*const create)(usize capacity_power_of_two);
    void (*const destroy)(cp_ringbuf_t *rb);
    b32 (*const push)(cp_ringbuf_t *rb, void *item);
    b32 (*const pop)(cp_ringbuf_t *rb, void **out_item);
    usize (*const count)(const cp_ringbuf_t *rb);
} cp_ringbuf_namespace_t;

CP_INLINE usize _cp_vec_ns_len(const void *vec) { return cp_vec_len(vec); }
CP_INLINE usize _cp_vec_ns_cap(const void *vec) { return cp_vec_cap(vec); }
CP_INLINE usize _cp_slist_ns_len(const cp_slist_t *list) { return list ? list->len : 0; }
CP_INLINE usize _cp_dlist_ns_len(const cp_dlist_t *list) { return list ? list->len : 0; }

static const cp_vec_namespace_t vec = {
    .len = _cp_vec_ns_len,
    .cap = _cp_vec_ns_cap,
    .free = _cp_vec_free,
};

static const cp_slist_namespace_t list = {
    .create = cp_slist_new,
    .pushFront = cp_slist_push_front,
    .pushBack = cp_slist_push_back,
    .popFront = cp_slist_pop_front,
    .len = _cp_slist_ns_len,
    .clear = cp_slist_clear,
};

static const cp_dlist_namespace_t dlist = {
    .create = cp_dlist_new,
    .pushFront = cp_dlist_push_front,
    .pushBack = cp_dlist_push_back,
    .popFront = cp_dlist_pop_front,
    .popBack = cp_dlist_pop_back,
    .remove = cp_dlist_remove,
    .insertAfter = cp_dlist_insert_after,
    .len = _cp_dlist_ns_len,
    .clear = cp_dlist_clear,
};

static const cp_ringbuf_namespace_t ringbuf = {
    .create = cp_ringbuf_create,
    .destroy = cp_ringbuf_destroy,
    .push = cp_ringbuf_push,
    .pop = cp_ringbuf_pop,
    .count = cp_ringbuf_count,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_graph --- */
/**
 * @file cp_graph.h
 * @brief Zero-latency Graph data structure (Adjacency List on Arena) with BFS, DFS,
 *        and Min-Heap Dijkstra Shortest Path algorithm.
 */



#ifdef __cplusplus
extern "C" {
#endif

typedef struct cp_graph_edge {
    usize to;
    f64 weight;
    struct cp_graph_edge *next;
} cp_graph_edge_t;

typedef cp_graph_edge_t GraphEdge;

typedef struct cp_graph {
    usize node_count;
    usize edge_count;
    cp_graph_edge_t **adj;
    b32 is_directed;
    cp_arena_t *arena;
} cp_graph_t;

typedef cp_graph_t Graph;

typedef void (*cp_graph_visitor_fn)(usize node, void *user_data);

/* -------------------------------------------------------------------------
 * Graph Construction & Query Operations
 * ------------------------------------------------------------------------- */
cp_graph_t cp_graph_new(cp_arena_t *arena, usize node_count, b32 is_directed);
void cp_graph_add_edge(cp_graph_t *g, usize from, usize to, f64 weight);
usize cp_graph_node_count(const cp_graph_t *g);
usize cp_graph_edge_count(const cp_graph_t *g);

/* -------------------------------------------------------------------------
 * Graph Traversal & Shortest Path Algorithms
 * ------------------------------------------------------------------------- */
void cp_graph_bfs(const cp_graph_t *g, usize start_node, cp_graph_visitor_fn visitor, void *user_data);
void cp_graph_dfs(const cp_graph_t *g, usize start_node, cp_graph_visitor_fn visitor, void *user_data);

/**
 * @brief Computes shortest path using Min-Heap optimized Dijkstra algorithm.
 * @param g Graph instance
 * @param start Source node index
 * @param target Destination node index
 * @param out_dist Optional pointer to store computed shortest distance
 * @param out_path Optional pointer to dynamic Vec(usize) populated with node sequence
 * @return true if path exists from start to target, false otherwise
 */
b32 cp_graph_dijkstra(const cp_graph_t *g, usize start, usize target, f64 *out_dist, Vec(usize) *out_path);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define GraphNew(arena, nodes, directed) cp_graph_new(arena, nodes, directed)
#define GraphAddEdge(g, u, v, w)         cp_graph_add_edge(g, u, v, w)
#define GraphNodeCount(g)                cp_graph_node_count(g)
#define GraphEdgeCount(g)                cp_graph_edge_count(g)
#define GraphBfs(g, start, fn, data)     cp_graph_bfs(g, start, fn, data)
#define GraphDfs(g, start, fn, data)     cp_graph_dfs(g, start, fn, data)
#define GraphDijkstra(g, s, t, dist, p)  cp_graph_dijkstra(g, s, t, dist, p)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_graph_t (*const create)(cp_arena_t *arena, usize node_count, b32 is_directed);
    void (*const addEdge)(cp_graph_t *g, usize from, usize to, f64 weight);
    usize (*const nodeCount)(const cp_graph_t *g);
    usize (*const edgeCount)(const cp_graph_t *g);
    void (*const bfs)(const cp_graph_t *g, usize start, cp_graph_visitor_fn visitor, void *user_data);
    void (*const dfs)(const cp_graph_t *g, usize start, cp_graph_visitor_fn visitor, void *user_data);
    b32 (*const dijkstra)(const cp_graph_t *g, usize start, usize target, f64 *out_dist, Vec(usize) *out_path);
} cp_graph_namespace_t;

static const cp_graph_namespace_t graph = {
    .create = cp_graph_new,
    .addEdge = cp_graph_add_edge,
    .nodeCount = cp_graph_node_count,
    .edgeCount = cp_graph_edge_count,
    .bfs = cp_graph_bfs,
    .dfs = cp_graph_dfs,
    .dijkstra = cp_graph_dijkstra,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_algo --- */
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

/* --- Module: cp_math --- */
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

/* --- Module: cp_thread --- */
/**
 * @file cp_thread.h
 * @brief Zero-overhead threading primitives, scoped RAII locks, and lock-free/low-latency thread pool.
 */


#include <pthread.h>
#include <sched.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Thread Primitives
 * ------------------------------------------------------------------------- */
typedef pthread_t cp_thread_t;
typedef void *(*cp_thread_fn)(void *);

CP_INLINE bool cp_thread_create(cp_thread_t *t, cp_thread_fn fn, void *arg) {
    return pthread_create(t, NULL, fn, arg) == 0;
}

CP_INLINE bool cp_thread_join(cp_thread_t t, void **retval) {
    return pthread_join(t, retval) == 0;
}

CP_INLINE bool cp_thread_detach(cp_thread_t t) {
    return pthread_detach(t) == 0;
}

CP_INLINE cp_thread_t cp_thread_self(void) {
    return pthread_self();
}

CP_INLINE void cp_thread_yield(void) {
    sched_yield();
}

/* -------------------------------------------------------------------------
 * Mutex & Scoped RAII Locks
 * ------------------------------------------------------------------------- */
typedef pthread_mutex_t cp_mutex_t;
typedef cp_mutex_t Mutex;

CP_INLINE void cp_mutex_init(cp_mutex_t *m) {
    pthread_mutex_init(m, NULL);
}

CP_INLINE void cp_mutex_destroy(cp_mutex_t *m) {
    pthread_mutex_destroy(m);
}

CP_INLINE void cp_mutex_lock(cp_mutex_t *m) {
    pthread_mutex_lock(m);
}

CP_INLINE void cp_mutex_unlock(cp_mutex_t *m) {
    pthread_mutex_unlock(m);
}

CP_INLINE bool cp_mutex_trylock(cp_mutex_t *m) {
    return pthread_mutex_trylock(m) == 0;
}

static inline void _cp_scoped_mutex_unlock(pthread_mutex_t **m) {
    if (m && *m) {
        pthread_mutex_unlock(*m);
    }
}

/**
 * @brief Automatic RAII Scoped Lock for any lexical block.
 * Releases the mutex automatically when execution leaves the enclosing scope.
 */
#define ScopedLock(m_ptr) \
    pthread_mutex_t *CP_CONCAT(_scoped_mtx_, __LINE__) __attribute__((cleanup(_cp_scoped_mutex_unlock), unused)) = \
        (pthread_mutex_lock((pthread_mutex_t*)(m_ptr)), (pthread_mutex_t*)(m_ptr))

/**
 * @brief Block-scoped RAII lock: WithLock(&mutex) { ... }
 */
#define WithLock(m_ptr) \
    for (int _lock_done_ = (pthread_mutex_lock((pthread_mutex_t*)(m_ptr)), 0); \
         !_lock_done_; \
         pthread_mutex_unlock((pthread_mutex_t*)(m_ptr)), _lock_done_ = 1)

/* -------------------------------------------------------------------------
 * Condition Variables
 * ------------------------------------------------------------------------- */
typedef pthread_cond_t cp_cond_t;
typedef cp_cond_t CondVar;

CP_INLINE void cp_cond_init(cp_cond_t *c) {
    pthread_cond_init(c, NULL);
}

CP_INLINE void cp_cond_destroy(cp_cond_t *c) {
    pthread_cond_destroy(c);
}

CP_INLINE void cp_cond_wait(cp_cond_t *c, cp_mutex_t *m) {
    pthread_cond_wait(c, m);
}

CP_INLINE void cp_cond_signal(cp_cond_t *c) {
    pthread_cond_signal(c);
}

CP_INLINE void cp_cond_broadcast(cp_cond_t *c) {
    pthread_cond_broadcast(c);
}

bool cp_cond_timedwait_ms(cp_cond_t *c, cp_mutex_t *m, u32 timeout_ms);

/* -------------------------------------------------------------------------
 * Atomic Primitives (C11 / GCC builtins)
 * ------------------------------------------------------------------------- */
#define AtomicInc(ptr)        __atomic_add_fetch((ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicDec(ptr)        __atomic_sub_fetch((ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicAdd(ptr, val)   __atomic_add_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define AtomicSub(ptr, val)   __atomic_sub_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define AtomicLoad(ptr)       __atomic_load_n((ptr), __ATOMIC_SEQ_CST)
#define AtomicStore(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_SEQ_CST)

/* -------------------------------------------------------------------------
 * Low-Latency Thread Pool
 * ------------------------------------------------------------------------- */
typedef struct cp_threadpool cp_threadpool_t;
typedef cp_threadpool_t ThreadPool;

/**
 * @brief Create a pre-warmed worker thread pool.
 * @param num_threads Number of worker threads (0 = auto-detect hardware concurrency).
 * @param queue_capacity Maximum queued tasks before submit blocks (0 = default 4096).
 */
cp_threadpool_t *cp_threadpool_create(usize num_threads, usize queue_capacity);

/**
 * @brief Submit a task to the pool. Blocks if queue is full until space is available.
 */
bool cp_threadpool_submit(cp_threadpool_t *pool, void (*fn)(void *), void *arg);

/**
 * @brief Non-blocking attempt to submit a task. Returns false if queue is full.
 */
bool cp_threadpool_try_submit(cp_threadpool_t *pool, void (*fn)(void *), void *arg);

/**
 * @brief Block calling thread until all queued and running tasks have finished.
 */
void cp_threadpool_wait(cp_threadpool_t *pool);

/**
 * @brief Stop worker threads, drain pending queue, and free resources.
 */
void cp_threadpool_destroy(cp_threadpool_t *pool);

/**
 * @brief Get the number of worker threads in the pool.
 */
usize cp_threadpool_num_threads(const cp_threadpool_t *pool);

typedef void (*cp_parallel_for_fn)(usize start, usize end, void *arg);

/**
 * @brief Partition [0, count) across thread pool workers and synchronize on completion.
 * Calling thread participates in execution to maximize CPU utilization.
 */
void cp_threadpool_parallel_for(cp_threadpool_t *pool, usize count, cp_parallel_for_fn fn, void *arg);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define ThreadCreate(t, fn, arg)         cp_thread_create(t, fn, arg)
#define ThreadJoin(t, ret)               cp_thread_join(t, ret)
#define ThreadDetach(t)                  cp_thread_detach(t)
#define ThreadSelf()                     cp_thread_self()
#define ThreadYield()                    cp_thread_yield()

#define MutexInit(m)                     cp_mutex_init(m)
#define MutexDestroy(m)                  cp_mutex_destroy(m)
#define MutexLock(m)                     cp_mutex_lock(m)
#define MutexUnlock(m)                   cp_mutex_unlock(m)
#define MutexTryLock(m)                  cp_mutex_trylock(m)

#define CondInit(c)                      cp_cond_init(c)
#define CondDestroy(c)                   cp_cond_destroy(c)
#define CondWait(c, m)                   cp_cond_wait(c, m)
#define CondSignal(c)                    cp_cond_signal(c)
#define CondBroadcast(c)                 cp_cond_broadcast(c)
#define CondTimedWaitMs(c, m, ms)        cp_cond_timedwait_ms(c, m, ms)

#define ThreadPoolCreate(threads, cap)      cp_threadpool_create(threads, cap)
#define ThreadPoolSubmit(pool, fn, arg)     cp_threadpool_submit(pool, fn, arg)
#define ThreadPoolTrySubmit(p, fn, arg)     cp_threadpool_try_submit(p, fn, arg)
#define ThreadPoolWait(pool)                cp_threadpool_wait(pool)
#define ThreadPoolDestroy(pool)             cp_threadpool_destroy(pool)
#define ThreadPoolNumThreads(pool)          cp_threadpool_num_threads(pool)
#define ThreadPoolParallelFor(p, c, fn, a)  cp_threadpool_parallel_for(p, c, fn, a)
#define ParallelFor(pool, count, fn, arg)   cp_threadpool_parallel_for(pool, count, fn, arg)

#if defined(CP_HAS_LAMBDA) && CP_HAS_LAMBDA
/**
 * @brief In-place parallel vector mapping using thread pool and inlined closure.
 */
#define VecParallelMap(pool, v, body) do { \
    usize _vp_len = VecLen(v); \
    if ((pool) && _vp_len > 0) { \
        cp_threadpool_parallel_for((pool), _vp_len, Lambda(void, (usize _p_start, usize _p_end, void *_p_vec_raw), { \
            __typeof__(v) _p_arr = (__typeof__(v))_p_vec_raw; \
            for (usize _p_i = _p_start; _p_i < _p_end; _p_i++) { \
                __typeof__(v) it = &_p_arr[_p_i]; \
                CP_UNUSED(it); \
                body; \
            } \
        }), (v)); \
    } \
} while(0)
#endif

/* -------------------------------------------------------------------------
 * Dot-Notation Namespaces
 * ------------------------------------------------------------------------- */
typedef struct {
    bool (*const create)(cp_thread_t *t, cp_thread_fn fn, void *arg);
    bool (*const join)(cp_thread_t t, void **retval);
    bool (*const detach)(cp_thread_t t);
    cp_thread_t (*const self)(void);
    void (*const yield)(void);
} cp_thread_namespace_t;

typedef struct {
    void (*const init)(cp_mutex_t *m);
    void (*const destroy)(cp_mutex_t *m);
    void (*const lock)(cp_mutex_t *m);
    void (*const unlock)(cp_mutex_t *m);
    bool (*const trylock)(cp_mutex_t *m);
} cp_mutex_namespace_t;

typedef struct {
    cp_threadpool_t *(*const create)(usize num_threads, usize queue_capacity);
    bool (*const submit)(cp_threadpool_t *pool, void (*fn)(void *), void *arg);
    bool (*const trySubmit)(cp_threadpool_t *pool, void (*fn)(void *), void *arg);
    void (*const parallelFor)(cp_threadpool_t *pool, usize count, cp_parallel_for_fn fn, void *arg);
    usize (*const numThreads)(const cp_threadpool_t *pool);
    void (*const wait)(cp_threadpool_t *pool);
    void (*const destroy)(cp_threadpool_t *pool);
} cp_threadpool_namespace_t;

static const cp_thread_namespace_t thread = {
    .create = cp_thread_create,
    .join = cp_thread_join,
    .detach = cp_thread_detach,
    .self = cp_thread_self,
    .yield = cp_thread_yield,
};

static const cp_mutex_namespace_t mutex = {
    .init = cp_mutex_init,
    .destroy = cp_mutex_destroy,
    .lock = cp_mutex_lock,
    .unlock = cp_mutex_unlock,
    .trylock = cp_mutex_trylock,
};

static const cp_threadpool_namespace_t threadpool = {
    .create = cp_threadpool_create,
    .submit = cp_threadpool_submit,
    .trySubmit = cp_threadpool_try_submit,
    .parallelFor = cp_threadpool_parallel_for,
    .numThreads = cp_threadpool_num_threads,
    .wait = cp_threadpool_wait,
    .destroy = cp_threadpool_destroy,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_net --- */
/**
 * @file cp_net.h
 * @brief High-performance networking primitives, non-blocking TCP/UDP, and Modbus/CAN industrial protocols.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Socket Descriptors & Network Configuration
 * ------------------------------------------------------------------------- */
typedef int cp_socket_t;
typedef cp_socket_t Socket;
#define CP_INVALID_SOCKET (-1)

void cp_net_close(cp_socket_t fd);
bool cp_net_set_nonblocking(cp_socket_t fd, bool nonblocking);
bool cp_net_set_reuseaddr(cp_socket_t fd, bool reuse);
bool cp_net_set_tcp_nodelay(cp_socket_t fd, bool nodelay);

/* -------------------------------------------------------------------------
 * TCP Sockets
 * ------------------------------------------------------------------------- */
cp_socket_t cp_net_tcp_connect(const char *host, u16 port, u32 timeout_ms);
cp_socket_t cp_net_tcp_listen(const char *bind_addr, u16 port, i32 backlog);
cp_socket_t cp_net_tcp_accept(cp_socket_t listen_fd, char *client_ip_out, usize ip_len, u16 *client_port_out);

i64 cp_net_send_all(cp_socket_t fd, const void *buf, usize len, u32 timeout_ms);
i64 cp_net_recv(cp_socket_t fd, void *buf, usize cap, u32 timeout_ms);

/**
 * @brief Ergonomic one-liner: connect, send request, receive full response, and close.
 */
bool cp_net_tcp_request(const char *host, u16 port, const void *req, usize req_len,
                        void *resp_buf, usize resp_buf_cap, usize *resp_len_out, u32 timeout_ms);

/* -------------------------------------------------------------------------
 * UDP Sockets
 * ------------------------------------------------------------------------- */
cp_socket_t cp_net_udp_bind(const char *bind_addr, u16 port);
i64 cp_net_udp_send_to(cp_socket_t fd, const char *host, u16 port, const void *data, usize len);
i64 cp_net_udp_recv_from(cp_socket_t fd, void *buf, usize cap, char *sender_ip, usize ip_len, u16 *sender_port, u32 timeout_ms);

/**
 * @brief Ergonomic one-liner: send UDP packet to target without manual socket lifecycle.
 */
bool cp_net_udp_send(const char *host, u16 port, const void *data, usize len);

/* -------------------------------------------------------------------------
 * Industrial Protocols: Modbus (RTU & TCP)
 * ------------------------------------------------------------------------- */
/**
 * @brief Hardware-lookup fast Modbus CRC16 (poly 0xA001, init 0xFFFF).
 */
u16 cp_modbus_crc16(const u8 *buf, usize len);

/* Modbus RTU framing */
usize cp_modbus_rtu_build_read_holding_registers(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count);
usize cp_modbus_rtu_build_write_single_register(u8 *out, usize cap, u8 slave_id, u16 reg_addr, u16 value);
usize cp_modbus_rtu_build_write_multiple_registers(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count, const u16 *values);
bool cp_modbus_rtu_verify_crc(const u8 *frame, usize len);
bool cp_modbus_rtu_parse_read_response(const u8 *frame, usize len, u8 expected_slave, u16 *out_regs, u16 max_regs, u16 *out_reg_count);

/* Modbus TCP framing (MBAP Header + PDU) */
usize cp_modbus_tcp_build_read_holding_registers(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count);
usize cp_modbus_tcp_build_write_multiple_registers(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count, const u16 *values);
bool cp_modbus_tcp_parse_read_response(const u8 *frame, usize len, u16 expected_tx_id, u8 expected_unit_id, u16 *out_regs, u16 max_regs, u16 *out_reg_count);

/* -------------------------------------------------------------------------
 * Industrial Protocols: CAN Bus (SocketCAN & Frame Packing)
 * ------------------------------------------------------------------------- */
typedef struct {
    u32 can_id;       /**< 11-bit standard or 29-bit extended ID */
    u8  can_dlc;      /**< Length code: 0..8 bytes */
    u8  data[8];      /**< Payload */
    bool is_extended; /**< True for 29-bit extended frame */
    bool is_rtr;      /**< True for Remote Transmission Request */
} cp_can_frame_t;

/* Portable frame pack/unpack */
void cp_can_pack(cp_can_frame_t *frame, u32 id, const void *data, u8 len, bool is_extended);
bool cp_can_unpack(const cp_can_frame_t *frame, u32 *out_id, void *out_data, u8 *out_len, bool *out_is_extended);

/* Linux SocketCAN native network interface */
cp_socket_t cp_can_open(const char *iface_name);
bool cp_can_send(cp_socket_t fd, const cp_can_frame_t *frame);
bool cp_can_recv(cp_socket_t fd, cp_can_frame_t *out_frame, u32 timeout_ms);
bool cp_can_set_filter(cp_socket_t fd, u32 can_id, u32 can_mask);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define NetClose(fd)                         cp_net_close(fd)
#define NetSetNonblocking(fd, nb)            cp_net_set_nonblocking(fd, nb)
#define NetSetReuseAddr(fd, r)               cp_net_set_reuseaddr(fd, r)
#define NetSetTcpNoDelay(fd, nd)             cp_net_set_tcp_nodelay(fd, nd)

#define NetTcpConnect(h, p, to)              cp_net_tcp_connect(h, p, to)
#define NetTcpListen(addr, p, bl)            cp_net_tcp_listen(addr, p, bl)
#define NetTcpAccept(lfd, ip, ipl, port)     cp_net_tcp_accept(lfd, ip, ipl, port)
#define NetSendAll(fd, buf, len, to)         cp_net_send_all(fd, buf, len, to)
#define NetRecv(fd, buf, cap, to)            cp_net_recv(fd, buf, cap, to)
#define NetTcpRequest(h, p, r, rl, rb, rbc, ro, to) cp_net_tcp_request(h, p, r, rl, rb, rbc, ro, to)

#define NetUdpBind(addr, p)                  cp_net_udp_bind(addr, p)
#define NetUdpSendTo(fd, h, p, d, l)         cp_net_udp_send_to(fd, h, p, d, l)
#define NetUdpRecvFrom(fd, b, c, ip, il, p, to) cp_net_udp_recv_from(fd, b, c, ip, il, p, to)
#define NetUdpSend(h, p, d, l)               cp_net_udp_send(h, p, d, l)

#define ModbusCrc16(b, l)                    cp_modbus_crc16(b, l)
#define ModbusRtuBuildRead(o, c, s, a, n)    cp_modbus_rtu_build_read_holding_registers(o, c, s, a, n)
#define ModbusRtuBuildWriteSingle(o, c, s, a, v) cp_modbus_rtu_build_write_single_register(o, c, s, a, v)
#define ModbusRtuBuildWriteMultiple(o, c, s, a, n, v) cp_modbus_rtu_build_write_multiple_registers(o, c, s, a, n, v)
#define ModbusRtuVerifyCrc(f, l)             cp_modbus_rtu_verify_crc(f, l)
#define ModbusRtuParseRead(f, l, s, o, m, c) cp_modbus_rtu_parse_read_response(f, l, s, o, m, c)

#define ModbusTcpBuildRead(o, c, tx, u, a, n) cp_modbus_tcp_build_read_holding_registers(o, c, tx, u, a, n)
#define ModbusTcpBuildWriteMultiple(o, c, tx, u, a, n, v) cp_modbus_tcp_build_write_multiple_registers(o, c, tx, u, a, n, v)
#define ModbusTcpParseRead(f, l, tx, u, o, m, c) cp_modbus_tcp_parse_read_response(f, l, tx, u, o, m, c)

#define CanPack(f, id, d, l, ext)            cp_can_pack(f, id, d, l, ext)
#define CanUnpack(f, id, d, l, ext)          cp_can_unpack(f, id, d, l, ext)
#define CanOpen(iface)                       cp_can_open(iface)
#define CanSend(fd, f)                       cp_can_send(fd, f)
#define CanRecv(fd, f, to)                   cp_can_recv(fd, f, to)
#define CanSetFilter(fd, id, mask)           cp_can_set_filter(fd, id, mask)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespaces
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*const close)(cp_socket_t fd);
    bool (*const setNonblocking)(cp_socket_t fd, bool nonblocking);
    bool (*const setReuseAddr)(cp_socket_t fd, bool reuse);
    bool (*const setTcpNoDelay)(cp_socket_t fd, bool nodelay);
    cp_socket_t (*const tcpConnect)(const char *host, u16 port, u32 timeout_ms);
    cp_socket_t (*const tcpListen)(const char *bind_addr, u16 port, i32 backlog);
    cp_socket_t (*const tcpAccept)(cp_socket_t listen_fd, char *client_ip_out, usize ip_len, u16 *client_port_out);
    i64 (*const sendAll)(cp_socket_t fd, const void *buf, usize len, u32 timeout_ms);
    i64 (*const recv)(cp_socket_t fd, void *buf, usize cap, u32 timeout_ms);
    bool (*const tcpRequest)(const char *host, u16 port, const void *req, usize req_len,
                             void *resp_buf, usize resp_buf_cap, usize *resp_len_out, u32 timeout_ms);
    cp_socket_t (*const udpBind)(const char *bind_addr, u16 port);
    i64 (*const udpSendTo)(cp_socket_t fd, const char *host, u16 port, const void *data, usize len);
    i64 (*const udpRecvFrom)(cp_socket_t fd, void *buf, usize cap, char *sender_ip, usize ip_len, u16 *sender_port, u32 timeout_ms);
    bool (*const udpSend)(const char *host, u16 port, const void *data, usize len);
} cp_net_namespace_t;

typedef struct {
    u16 (*const crc16)(const u8 *buf, usize len);
    usize (*const rtuBuildRead)(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count);
    usize (*const rtuBuildWriteSingle)(u8 *out, usize cap, u8 slave_id, u16 reg_addr, u16 value);
    usize (*const rtuBuildWriteMultiple)(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count, const u16 *values);
    bool (*const rtuVerifyCrc)(const u8 *frame, usize len);
    bool (*const rtuParseRead)(const u8 *frame, usize len, u8 expected_slave, u16 *out_regs, u16 max_regs, u16 *out_reg_count);
    usize (*const tcpBuildRead)(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count);
    usize (*const tcpBuildWriteMultiple)(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count, const u16 *values);
    bool (*const tcpParseRead)(const u8 *frame, usize len, u16 expected_tx_id, u8 expected_unit_id, u16 *out_regs, u16 max_regs, u16 *out_reg_count);
} cp_modbus_namespace_t;

typedef struct {
    void (*const pack)(cp_can_frame_t *frame, u32 id, const void *data, u8 len, bool is_extended);
    bool (*const unpack)(const cp_can_frame_t *frame, u32 *out_id, void *out_data, u8 *out_len, bool *out_is_extended);
    cp_socket_t (*const open)(const char *iface_name);
    bool (*const send)(cp_socket_t fd, const cp_can_frame_t *frame);
    bool (*const recv)(cp_socket_t fd, cp_can_frame_t *out_frame, u32 timeout_ms);
    bool (*const setFilter)(cp_socket_t fd, u32 can_id, u32 can_mask);
} cp_can_namespace_t;

static const cp_net_namespace_t net = {
    .close = cp_net_close,
    .setNonblocking = cp_net_set_nonblocking,
    .setReuseAddr = cp_net_set_reuseaddr,
    .setTcpNoDelay = cp_net_set_tcp_nodelay,
    .tcpConnect = cp_net_tcp_connect,
    .tcpListen = cp_net_tcp_listen,
    .tcpAccept = cp_net_tcp_accept,
    .sendAll = cp_net_send_all,
    .recv = cp_net_recv,
    .tcpRequest = cp_net_tcp_request,
    .udpBind = cp_net_udp_bind,
    .udpSendTo = cp_net_udp_send_to,
    .udpRecvFrom = cp_net_udp_recv_from,
    .udpSend = cp_net_udp_send,
};

static const cp_modbus_namespace_t modbus = {
    .crc16 = cp_modbus_crc16,
    .rtuBuildRead = cp_modbus_rtu_build_read_holding_registers,
    .rtuBuildWriteSingle = cp_modbus_rtu_build_write_single_register,
    .rtuBuildWriteMultiple = cp_modbus_rtu_build_write_multiple_registers,
    .rtuVerifyCrc = cp_modbus_rtu_verify_crc,
    .rtuParseRead = cp_modbus_rtu_parse_read_response,
    .tcpBuildRead = cp_modbus_tcp_build_read_holding_registers,
    .tcpBuildWriteMultiple = cp_modbus_tcp_build_write_multiple_registers,
    .tcpParseRead = cp_modbus_tcp_parse_read_response,
};

static const cp_can_namespace_t can = {
    .pack = cp_can_pack,
    .unpack = cp_can_unpack,
    .open = cp_can_open,
    .send = cp_can_send,
    .recv = cp_can_recv,
    .setFilter = cp_can_set_filter,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_tui --- */
/**
 * @file cp_tui.h
 * @brief High-performance, zero-dependency double-buffered Terminal UI (TUI) engine.
 *        Features sub-microsecond differential ANSI updates, Unicode boxes, progress bars,
 *        and crash-safe terminal restoration.
 */


#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Color Definitions (ANSI 16-color palette)
 * ------------------------------------------------------------------------- */
#define TUI_COLOR_DEFAULT        0
#define TUI_COLOR_BLACK          1
#define TUI_COLOR_RED            2
#define TUI_COLOR_GREEN          3
#define TUI_COLOR_YELLOW         4
#define TUI_COLOR_BLUE           5
#define TUI_COLOR_MAGENTA        6
#define TUI_COLOR_CYAN           7
#define TUI_COLOR_WHITE          8
#define TUI_COLOR_BRIGHT_BLACK   9
#define TUI_COLOR_BRIGHT_RED     10
#define TUI_COLOR_BRIGHT_GREEN   11
#define TUI_COLOR_BRIGHT_YELLOW  12
#define TUI_COLOR_BRIGHT_BLUE    13
#define TUI_COLOR_BRIGHT_MAGENTA 14
#define TUI_COLOR_BRIGHT_CYAN    15
#define TUI_COLOR_BRIGHT_WHITE   16

/* -------------------------------------------------------------------------
 * Text Attributes
 * ------------------------------------------------------------------------- */
#define TUI_ATTR_NONE            0x00
#define TUI_ATTR_BOLD            0x01
#define TUI_ATTR_DIM             0x02
#define TUI_ATTR_UNDERLINE       0x04
#define TUI_ATTR_REVERSE         0x08

/* -------------------------------------------------------------------------
 * Input Key Codes
 * ------------------------------------------------------------------------- */
typedef enum {
    TUI_KEY_NONE      = 0,
    TUI_KEY_UP        = 1000,
    TUI_KEY_DOWN,
    TUI_KEY_LEFT,
    TUI_KEY_RIGHT,
    TUI_KEY_ENTER,
    TUI_KEY_ESCAPE,
    TUI_KEY_BACKSPACE,
    TUI_KEY_TAB
} cp_tui_key_t;

typedef cp_tui_key_t TuiKey;

/* -------------------------------------------------------------------------
 * Cell & TUI Matrix Structures
 * ------------------------------------------------------------------------- */
typedef struct {
    char glyph[8]; /**< UTF-8 multi-byte or ASCII glyph (null-terminated) */
    u8 fg;         /**< Foreground color */
    u8 bg;         /**< Background color */
    u8 attrs;      /**< Attributes bitmask */
    u8 _pad;
} cp_tui_cell_t;

typedef cp_tui_cell_t TuiCell;

typedef struct cp_tui {
    usize width;
    usize height;
    cp_tui_cell_t *front_buffer;
    cp_tui_cell_t *back_buffer;
    char *out_buffer;
    usize out_cap;
    usize out_len;
    bool is_headless;
    bool raw_mode_active;
    struct termios orig_termios;
} cp_tui_t;

typedef cp_tui_t Tui;

/* -------------------------------------------------------------------------
 * Lifecycle Management
 * ------------------------------------------------------------------------- */
/**
 * @brief Initialize TUI on physical terminal.
 * @param width Desired width (0 = auto-detect terminal size).
 * @param height Desired height (0 = auto-detect terminal size).
 */
cp_tui_t *cp_tui_init(usize width, usize height);

/**
 * @brief Create a headless TUI engine for off-screen rendering and automated tests.
 */
cp_tui_t *cp_tui_create(usize width, usize height, bool is_headless);

/**
 * @brief Shutdown TUI, restore terminal mode and cursor, and free memory.
 */
void cp_tui_shutdown(cp_tui_t *tui);

/* -------------------------------------------------------------------------
 * Rendering & Differential Flusher
 * ------------------------------------------------------------------------- */
/**
 * @brief Clear the back buffer with default or specified background color.
 */
void cp_tui_clear(cp_tui_t *tui, u8 bg);

/**
 * @brief Compare back buffer with front buffer and flush only changed cells.
 * @return Number of characters written to the output stream.
 */
usize cp_tui_present(cp_tui_t *tui);

/* -------------------------------------------------------------------------
 * Drawing Primitives
 * ------------------------------------------------------------------------- */
void cp_tui_set_cell(cp_tui_t *tui, usize x, usize y, const char *glyph, u8 fg, u8 bg, u8 attrs);
void cp_tui_print(cp_tui_t *tui, usize x, usize y, const char *str, u8 fg, u8 bg, u8 attrs);
void cp_tui_box(cp_tui_t *tui, usize x, usize y, usize w, usize h, const char *title, u8 fg, u8 bg);
void cp_tui_progress_bar(cp_tui_t *tui, usize x, usize y, usize width, f32 percentage, const char *label, u8 fg, u8 bg);

/* -------------------------------------------------------------------------
 * Interactive Components & Input
 * ------------------------------------------------------------------------- */
cp_tui_key_t cp_tui_read_key(cp_tui_t *tui, u32 timeout_ms);
bool cp_tui_menu(cp_tui_t *tui, usize x, usize y, usize w, const char *title, const char **options, usize count, usize *selected_index);
bool cp_tui_prompt(cp_tui_t *tui, usize x, usize y, usize w, const char *prompt, char *out_buf, usize cap);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define TuiInit(w, h)                            cp_tui_init(w, h)
#define TuiCreate(w, h, headless)                cp_tui_create(w, h, headless)
#define TuiShutdown(tui)                         cp_tui_shutdown(tui)
#define TuiClear(tui, bg)                        cp_tui_clear(tui, bg)
#define TuiPresent(tui)                          cp_tui_present(tui)
#define TuiSetCell(tui, x, y, g, fg, bg, a)      cp_tui_set_cell(tui, x, y, g, fg, bg, a)
#define TuiPrint(tui, x, y, str, fg, bg, a)      cp_tui_print(tui, x, y, str, fg, bg, a)
#define TuiBox(tui, x, y, w, h, t, fg, bg)       cp_tui_box(tui, x, y, w, h, t, fg, bg)
#define TuiProgressBar(tui, x, y, w, p, l, fg, bg) cp_tui_progress_bar(tui, x, y, w, p, l, fg, bg)
#define TuiReadKey(tui, ms)                      cp_tui_read_key(tui, ms)
#define TuiMenu(tui, x, y, w, t, opt, cnt, idx)  cp_tui_menu(tui, x, y, w, t, opt, cnt, idx)
#define TuiPrompt(tui, x, y, w, p, buf, cap)     cp_tui_prompt(tui, x, y, w, p, buf, cap)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_tui_t *(*const init)(usize width, usize height);
    cp_tui_t *(*const create)(usize width, usize height, bool is_headless);
    void (*const shutdown)(cp_tui_t *tui);
    void (*const clear)(cp_tui_t *tui, u8 bg);
    usize (*const present)(cp_tui_t *tui);
    void (*const setCell)(cp_tui_t *tui, usize x, usize y, const char *glyph, u8 fg, u8 bg, u8 attrs);
    void (*const print)(cp_tui_t *tui, usize x, usize y, const char *str, u8 fg, u8 bg, u8 attrs);
    void (*const box)(cp_tui_t *tui, usize x, usize y, usize w, usize h, const char *title, u8 fg, u8 bg);
    void (*const progressBar)(cp_tui_t *tui, usize x, usize y, usize width, f32 percentage, const char *label, u8 fg, u8 bg);
    cp_tui_key_t (*const readKey)(cp_tui_t *tui, u32 timeout_ms);
    bool (*const menu)(cp_tui_t *tui, usize x, usize y, usize w, const char *title, const char **options, usize count, usize *selected_index);
    bool (*const prompt)(cp_tui_t *tui, usize x, usize y, usize w, const char *prompt, char *out_buf, usize cap);
} cp_tui_namespace_t;

static const cp_tui_namespace_t tui = {
    .init = cp_tui_init,
    .create = cp_tui_create,
    .shutdown = cp_tui_shutdown,
    .clear = cp_tui_clear,
    .present = cp_tui_present,
    .setCell = cp_tui_set_cell,
    .print = cp_tui_print,
    .box = cp_tui_box,
    .progressBar = cp_tui_progress_bar,
    .readKey = cp_tui_read_key,
    .menu = cp_tui_menu,
    .prompt = cp_tui_prompt,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_hash --- */
/**
 * @file cp_hash.h
 * @brief Ultra-fast non-cryptographic (FNV-1a, Hash64) and cryptographic (SHA-256) hash functions.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Non-Cryptographic Fast Hashes
 * ------------------------------------------------------------------------- */
/**
 * @brief 32-bit FNV-1a hash.
 */
u32 cp_hash_fnv1a32(const void *data, usize len);

/**
 * @brief 64-bit FNV-1a hash.
 */
u64 cp_hash_fnv1a64(const void *data, usize len);

/**
 * @brief Blindingly fast 64-bit non-cryptographic hash (< 1 ns/byte).
 */
u64 cp_hash64(const void *data, usize len);

/* -------------------------------------------------------------------------
 * Cryptographic Hash: SHA-256 (FIPS 180-4 Standard)
 * ------------------------------------------------------------------------- */
typedef struct {
    u32 state[8];
    u64 count;
    u8 buffer[64];
} cp_sha256_ctx_t;

void cp_sha256_init(cp_sha256_ctx_t *ctx);
void cp_sha256_update(cp_sha256_ctx_t *ctx, const void *data, usize len);
void cp_sha256_final(cp_sha256_ctx_t *ctx, u8 out_hash[32]);

/**
 * @brief One-liner compute SHA-256 hash.
 */
void cp_sha256(const void *data, usize len, u8 out_hash[32]);

/**
 * @brief Format 32-byte SHA-256 hash as 64-character lowercase hexadecimal string (null-terminated).
 */
void cp_sha256_hex(const u8 in_hash[32], char out_hex[65]);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define HashFnv1a32(data, len)          cp_hash_fnv1a32(data, len)
#define HashFnv1a64(data, len)          cp_hash_fnv1a64(data, len)
#define Hash64(data, len)               cp_hash64(data, len)

#define Sha256(data, len, out)          cp_sha256(data, len, out)
#define Sha256Hex(in_hash, out_hex)     cp_sha256_hex(in_hash, out_hex)
#define Sha256Init(ctx)                 cp_sha256_init(ctx)
#define Sha256Update(ctx, data, len)    cp_sha256_update(ctx, data, len)
#define Sha256Final(ctx, out)           cp_sha256_final(ctx, out)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    u32 (*const fnv1a32)(const void *data, usize len);
    u64 (*const fnv1a64)(const void *data, usize len);
    u64 (*const u64_hash)(const void *data, usize len);
    void (*const sha256)(const void *data, usize len, u8 out_hash[32]);
    void (*const sha256Hex)(const u8 in_hash[32], char out_hex[65]);
} cp_hash_namespace_t;

static const cp_hash_namespace_t hash_api = {
    .fnv1a32 = cp_hash_fnv1a32,
    .fnv1a64 = cp_hash_fnv1a64,
    .u64_hash = cp_hash64,
    .sha256 = cp_sha256,
    .sha256Hex = cp_sha256_hex,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_map --- */
/**
 * @file cp_map.h
 * @brief High-performance, open-addressing Hash Map with linear probing and optional Arena backing.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Hash Map Data Structures
 * ------------------------------------------------------------------------- */
typedef struct {
    char *key;
    void *value;
    u64 hash;
    bool occupied;
    bool deleted;
} cp_map_entry_t;

typedef cp_map_entry_t MapEntry;

typedef struct cp_map {
    cp_map_entry_t *entries;
    usize capacity;
    usize count;
    usize tombstones;
    cp_arena_t *arena;
} cp_map_t;

typedef cp_map_t HashMap;

/* -------------------------------------------------------------------------
 * Core Map Operations
 * ------------------------------------------------------------------------- */
cp_map_t *cp_map_create(usize initial_capacity, cp_arena_t *arena);
void cp_map_destroy(cp_map_t *map);

bool cp_map_set(cp_map_t *map, const char *key, void *value);
void *cp_map_get(const cp_map_t *map, const char *key);
bool cp_map_has(const cp_map_t *map, const char *key);
bool cp_map_delete(cp_map_t *map, const char *key);
void cp_map_clear(cp_map_t *map);
usize cp_map_len(const cp_map_t *map);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define MapCreate(cap, arena)           cp_map_create(cap, arena)
#define MapDestroy(map)                 cp_map_destroy(map)
#define MapSet(map, key, val)           cp_map_set(map, key, (void *)(val))
#define MapGet(map, key)                cp_map_get(map, key)
#define MapGetT(map, key, type)         ((type)cp_map_get(map, key))
#define MapHas(map, key)                cp_map_has(map, key)
#define MapDelete(map, key)             cp_map_delete(map, key)
#define MapClear(map)                   cp_map_clear(map)
#define MapLen(map)                     cp_map_len(map)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_map_t *(*const create)(usize initial_capacity, cp_arena_t *arena);
    void (*const destroy)(cp_map_t *map);
    bool (*const set)(cp_map_t *map, const char *key, void *value);
    void *(*const get)(const cp_map_t *map, const char *key);
    bool (*const has)(const cp_map_t *map, const char *key);
    bool (*const delete_entry)(cp_map_t *map, const char *key);
    void (*const clear)(cp_map_t *map);
    usize (*const len)(const cp_map_t *map);
} cp_map_namespace_t;

static const cp_map_namespace_t map = {
    .create = cp_map_create,
    .destroy = cp_map_destroy,
    .set = cp_map_set,
    .get = cp_map_get,
    .has = cp_map_has,
    .delete_entry = cp_map_delete,
    .clear = cp_map_clear,
    .len = cp_map_len,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_args --- */
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

/* --- Module: cp_time --- */
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

/* --- Module: cp_json --- */
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

/* --- Module: cp_stack --- */
/**
 * @file cp_stack.h
 * @brief High-performance L1 cache-friendly Stack data structure with push, pop, and index-from-top get.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Stack Data Structure
 * ------------------------------------------------------------------------- */
typedef struct cp_stack {
    void **items;
    usize capacity;
    usize top;
    cp_arena_t *arena;
} cp_stack_t;

typedef cp_stack_t Stack;

/* -------------------------------------------------------------------------
 * Stack Operations
 * ------------------------------------------------------------------------- */
cp_stack_t *cp_stack_create(usize initial_capacity, cp_arena_t *arena);
void cp_stack_destroy(cp_stack_t *stack);

bool cp_stack_push(cp_stack_t *stack, void *item);
void *cp_stack_pop(cp_stack_t *stack);
void *cp_stack_get(const cp_stack_t *stack, usize index_from_top);
void *cp_stack_peek(const cp_stack_t *stack);
usize cp_stack_len(const cp_stack_t *stack);
bool cp_stack_is_empty(const cp_stack_t *stack);
void cp_stack_clear(cp_stack_t *stack);

/* Inlined fast-path zero-latency operations */
CP_INLINE bool cp_stack_push_fast(cp_stack_t *stack, void *item) {
    if (CP_LIKELY(stack && stack->top < stack->capacity)) {
        stack->items[stack->top++] = item;
        return true;
    }
    return cp_stack_push(stack, item);
}

CP_INLINE void *cp_stack_pop_fast(cp_stack_t *stack) {
    if (CP_LIKELY(stack && stack->top > 0)) {
        return stack->items[--stack->top];
    }
    return NULL;
}

CP_INLINE void *cp_stack_get_fast(const cp_stack_t *stack, usize index_from_top) {
    if (CP_LIKELY(stack && index_from_top < stack->top)) {
        return stack->items[stack->top - 1 - index_from_top];
    }
    return NULL;
}

CP_INLINE void *cp_stack_peek_fast(const cp_stack_t *stack) {
    return cp_stack_get_fast(stack, 0);
}

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define StackCreate(cap, arena)          cp_stack_create(cap, arena)
#define StackDestroy(s)                  cp_stack_destroy(s)
#define StackPush(s, item)               cp_stack_push_fast(s, (void *)(item))
#define StackPop(s)                      cp_stack_pop_fast(s)
#define StackPopT(s, type)               ((type)cp_stack_pop_fast(s))
#define StackGet(s, idx)                 cp_stack_get_fast(s, idx)
#define StackGetT(s, idx, type)          ((type)cp_stack_get_fast(s, idx))
#define StackPeek(s)                     cp_stack_peek_fast(s)
#define StackPeekT(s, type)              ((type)cp_stack_peek_fast(s))
#define StackLen(s)                      cp_stack_len(s)
#define StackIsEmpty(s)                  cp_stack_is_empty(s)
#define StackClear(s)                    cp_stack_clear(s)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_stack_t *(*const create)(usize initial_capacity, cp_arena_t *arena);
    void (*const destroy)(cp_stack_t *stack);
    bool (*const push)(cp_stack_t *stack, void *item);
    void *(*const pop)(cp_stack_t *stack);
    void *(*const get)(const cp_stack_t *stack, usize index_from_top);
    void *(*const peek)(const cp_stack_t *stack);
    usize (*const len)(const cp_stack_t *stack);
    bool (*const isEmpty)(const cp_stack_t *stack);
    void (*const clear)(cp_stack_t *stack);
} cp_stack_namespace_t;

static const cp_stack_namespace_t stack = {
    .create = cp_stack_create,
    .destroy = cp_stack_destroy,
    .push = cp_stack_push_fast,
    .pop = cp_stack_pop_fast,
    .get = cp_stack_get_fast,
    .peek = cp_stack_peek_fast,
    .len = cp_stack_len,
    .isEmpty = cp_stack_is_empty,
    .clear = cp_stack_clear,
};

#ifdef __cplusplus
}
#endif

/* --- Module: cp_os --- */
/**
 * @file cp_os.h
 * @brief Zero-latency Operating System, Process, Hardware, and Environment utilities.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Process Execution & System Info
 * ------------------------------------------------------------------------- */

/**
 * @brief Executes a shell command synchronously and captures standard output.
 * @param cmd The shell command line to execute.
 * @param out_buf Buffer to store standard output (NULL-terminated).
 * @param out_cap Maximum capacity of out_buf.
 * @param exit_code Pointer to store the exit code of the process (can be NULL).
 * @return Number of bytes written to out_buf (excluding NULL terminator), or -1 on error.
 */
i64 cp_os_exec(const char *cmd, char *out_buf, usize out_cap, int *exit_code);

/**
 * @brief Returns the number of logical CPU cores online.
 */
u32 cp_os_num_cpus(void);

/**
 * @brief Returns total physical RAM in bytes.
 */
u64 cp_os_total_ram_bytes(void);

/**
 * @brief Returns currently available physical RAM in bytes.
 */
u64 cp_os_avail_ram_bytes(void);

/**
 * @brief Returns system virtual memory page size in bytes.
 */
usize cp_os_page_size(void);

/**
 * @brief Gets the system hostname.
 * @param buf Output buffer.
 * @param len Buffer length.
 * @return True if successfully retrieved, false otherwise.
 */
bool cp_os_hostname(char *buf, usize len);

/**
 * @brief Returns system uptime in seconds.
 */
u64 cp_os_uptime_sec(void);

/* -------------------------------------------------------------------------
 * Process Management & Signals
 * ------------------------------------------------------------------------- */

/**
 * @brief Returns the current process ID (PID).
 */
i32 cp_os_pid(void);

/**
 * @brief Returns the parent process ID (PPID).
 */
i32 cp_os_ppid(void);

/**
 * @brief Sends a signal to a process.
 * @param pid Process ID.
 * @param sig Signal number (e.g., SIGTERM, SIGKILL, 0 to test existence).
 * @return True on success, false on error.
 */
bool cp_os_kill(i32 pid, int sig);

/**
 * @brief Checks if a process with given PID is alive.
 */
bool cp_os_is_process_running(i32 pid);

/* -------------------------------------------------------------------------
 * Environment Variables
 * ------------------------------------------------------------------------- */

/**
 * @brief Gets an environment variable string, or NULL if unset.
 */
const char *cp_os_env_get(const char *name);

/**
 * @brief Gets an environment variable string, returning default_val if unset.
 */
const char *cp_os_env_get_or(const char *name, const char *default_val);

/**
 * @brief Gets an environment variable parsed as an integer, returning default_val if unset or invalid.
 */
i64 cp_os_env_get_int(const char *name, i64 default_val);

/**
 * @brief Sets an environment variable. Overwrites if already exists.
 * @return True on success, false on failure.
 */
bool cp_os_env_set(const char *name, const char *value);

/**
 * @brief Unsets an environment variable.
 * @return True on success, false on failure.
 */
bool cp_os_env_unset(const char *name);

/**
 * @brief Checks if an environment variable exists.
 */
bool cp_os_env_has(const char *name);

/* -------------------------------------------------------------------------
 * High-Resolution Sleep
 * ------------------------------------------------------------------------- */

/**
 * @brief Sleeps for the specified number of milliseconds without busy-waiting.
 */
void cp_os_sleep_ms(u32 ms);

/**
 * @brief Sleeps for the specified number of microseconds without busy-waiting.
 */
void cp_os_sleep_us(u64 us);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define OsExec(cmd, buf, cap, exit_code) cp_os_exec(cmd, buf, cap, exit_code)
#define OsNumCpus()                      cp_os_num_cpus()
#define OsTotalRamBytes()                cp_os_total_ram_bytes()
#define OsAvailRamBytes()                cp_os_avail_ram_bytes()
#define OsPageSize()                     cp_os_page_size()
#define OsHostname(buf, len)             cp_os_hostname(buf, len)
#define OsUptimeSec()                    cp_os_uptime_sec()

#define OsPid()                          cp_os_pid()
#define OsPpid()                         cp_os_ppid()
#define OsKill(pid, sig)                 cp_os_kill(pid, sig)
#define OsIsRunning(pid)                 cp_os_is_process_running(pid)

#define OsEnvGet(name)                   cp_os_env_get(name)
#define OsEnvGetOr(name, def)            cp_os_env_get_or(name, def)
#define OsEnvGetInt(name, def)           cp_os_env_get_int(name, def)
#define OsEnvSet(name, val)              cp_os_env_set(name, val)
#define OsEnvUnset(name)                 cp_os_env_unset(name)
#define OsEnvHas(name)                   cp_os_env_has(name)

#define OsSleepMs(ms)                    cp_os_sleep_ms(ms)
#define OsSleepUs(us)                    cp_os_sleep_us(us)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    i64 (*const exec)(const char *cmd, char *out_buf, usize out_cap, int *exit_code);
    u32 (*const numCpus)(void);
    u64 (*const totalRamBytes)(void);
    u64 (*const availRamBytes)(void);
    usize (*const pageSize)(void);
    bool (*const hostname)(char *buf, usize len);
    u64 (*const uptimeSec)(void);

    i32 (*const pid)(void);
    i32 (*const ppid)(void);
    bool (*const kill)(i32 pid, int sig);
    bool (*const isRunning)(i32 pid);

    const char *(*const envGet)(const char *name);
    const char *(*const envGetOr)(const char *name, const char *default_val);
    i64 (*const envGetInt)(const char *name, i64 default_val);
    bool (*const envSet)(const char *name, const char *value);
    bool (*const envUnset)(const char *name);
    bool (*const envHas)(const char *name);

    void (*const sleepMs)(u32 ms);
    void (*const sleepUs)(u64 us);
} cp_os_namespace_t;

static const cp_os_namespace_t os = {
    .exec = cp_os_exec,
    .numCpus = cp_os_num_cpus,
    .totalRamBytes = cp_os_total_ram_bytes,
    .availRamBytes = cp_os_avail_ram_bytes,
    .pageSize = cp_os_page_size,
    .hostname = cp_os_hostname,
    .uptimeSec = cp_os_uptime_sec,
    .pid = cp_os_pid,
    .ppid = cp_os_ppid,
    .kill = cp_os_kill,
    .isRunning = cp_os_is_process_running,
    .envGet = cp_os_env_get,
    .envGetOr = cp_os_env_get_or,
    .envGetInt = cp_os_env_get_int,
    .envSet = cp_os_env_set,
    .envUnset = cp_os_env_unset,
    .envHas = cp_os_env_has,
    .sleepMs = cp_os_sleep_ms,
    .sleepUs = cp_os_sleep_us,
};

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_AMALGAMATED_H */

/* =========================================================================
 * IMPLEMENTATIONS (#define CPLUS_IMPLEMENTATION or #define CP_IMPLEMENTATION)
 * ========================================================================= */
#if defined(CPLUS_IMPLEMENTATION) || defined(CP_IMPLEMENTATION)

#ifdef __cplusplus
extern "C" {
#endif

/* --- Source: cp_mem.c --- */
/**
 * @file cp_mem.c
 * @brief High-performance implementation of Arena, Scratch, Pool, and Tracked Heap allocators.
 */

#include <string.h>
#include <pthread.h>

#define CP_MIN_CHUNK_SIZE 4096

/* -------------------------------------------------------------------------
 * Memory Alignment Helpers
 * ------------------------------------------------------------------------- */
CP_INLINE uintptr_t _cp_align_up(uintptr_t ptr, usize alignment) {
    if (alignment == 0) alignment = CP_DEFAULT_ALIGNMENT;
    return (ptr + (alignment - 1)) & ~(uintptr_t)(alignment - 1);
}

/* -------------------------------------------------------------------------
 * Arena Implementation
 * ------------------------------------------------------------------------- */
static cp_arena_chunk_t *_cp_arena_new_chunk(usize capacity) {
    if (capacity < CP_MIN_CHUNK_SIZE) {
        capacity = CP_MIN_CHUNK_SIZE;
    }
    cp_arena_chunk_t *chunk = (cp_arena_chunk_t *)malloc(sizeof(cp_arena_chunk_t) + capacity);
    if (!chunk) {
        Panic("Out of memory in Arena chunk allocation");
    }
    chunk->next = NULL;
    chunk->capacity = capacity;
    chunk->offset = 0;
    return chunk;
}

cp_arena_t *cp_arena_create(usize default_chunk_size) {
    if (default_chunk_size < CP_MIN_CHUNK_SIZE) {
        default_chunk_size = CP_MIN_CHUNK_SIZE;
    }
    cp_arena_t *arena = (cp_arena_t *)malloc(sizeof(cp_arena_t));
    if (!arena) {
        Panic("Out of memory creating Arena structure");
    }
    arena->default_chunk_size = default_chunk_size;
    arena->total_allocated = 0;
    arena->first = _cp_arena_new_chunk(default_chunk_size);
    arena->current = arena->first;
    return arena;
}

void cp_arena_destroy(cp_arena_t *arena) {
    if (!arena) return;
    cp_arena_chunk_t *cur = arena->first;
    while (cur) {
        cp_arena_chunk_t *next = cur->next;
        free(cur);
        cur = next;
    }
    free(arena);
}

void *cp_arena_alloc_align(cp_arena_t *arena, usize size, usize alignment) {
    if (!arena || size == 0) return NULL;
    if (alignment == 0) alignment = CP_DEFAULT_ALIGNMENT;

    cp_arena_chunk_t *chunk = arena->current;
    uintptr_t base = (uintptr_t)chunk->data;
    uintptr_t current_ptr = base + chunk->offset;
    uintptr_t aligned_ptr = _cp_align_up(current_ptr, alignment);
    usize needed_offset = (aligned_ptr - base) + size;

    if (needed_offset > chunk->capacity) {
        /* Allocate a new chunk large enough */
        usize next_capacity = arena->default_chunk_size;
        if (size + alignment > next_capacity) {
            next_capacity = size + alignment + CP_MIN_CHUNK_SIZE;
        }

        cp_arena_chunk_t *new_chunk = _cp_arena_new_chunk(next_capacity);
        chunk->next = new_chunk;
        arena->current = new_chunk;
        chunk = new_chunk;

        base = (uintptr_t)chunk->data;
        aligned_ptr = _cp_align_up(base, alignment);
        needed_offset = (aligned_ptr - base) + size;
    }

    chunk->offset = needed_offset;
    arena->total_allocated += size;
    return (void *)aligned_ptr;
}

void *cp_arena_alloc(cp_arena_t *arena, usize size) {
    return cp_arena_alloc_align(arena, size, CP_DEFAULT_ALIGNMENT);
}

void *cp_arena_alloc_zero(cp_arena_t *arena, usize size) {
    void *ptr = cp_arena_alloc(arena, size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void cp_arena_reset(cp_arena_t *arena) {
    if (!arena) return;
    cp_arena_chunk_t *cur = arena->first;
    while (cur) {
        cur->offset = 0;
        cur = cur->next;
    }
    arena->current = arena->first;
    arena->total_allocated = 0;
}

cp_temp_arena_t cp_temp_begin(cp_arena_t *arena) {
    cp_temp_arena_t temp;
    temp.arena = arena;
    temp.chunk = arena ? arena->current : NULL;
    temp.offset = (arena && arena->current) ? arena->current->offset : 0;
    return temp;
}

void cp_temp_end(cp_temp_arena_t temp) {
    if (!temp.arena || !temp.chunk) return;
    temp.arena->current = temp.chunk;
    temp.chunk->offset = temp.offset;
}

/* -------------------------------------------------------------------------
 * Scratch Arena (Thread-Local Circular Buffer)
 * ------------------------------------------------------------------------- */
#define CP_SCRATCH_COUNT 2
#define CP_SCRATCH_DEFAULT_CAPACITY (256 * 1024) /* 256 KB per scratch */

static _Thread_local cp_arena_t *tl_scratch_arenas[CP_SCRATCH_COUNT] = {NULL, NULL};
static _Thread_local usize tl_scratch_index = 0;

cp_arena_t *cp_scratch_get(void) {
    if (!tl_scratch_arenas[0]) {
        for (int i = 0; i < CP_SCRATCH_COUNT; i++) {
            tl_scratch_arenas[i] = cp_arena_create(CP_SCRATCH_DEFAULT_CAPACITY);
        }
    }
    cp_arena_t *arena = tl_scratch_arenas[tl_scratch_index];
    tl_scratch_index = (tl_scratch_index + 1) % CP_SCRATCH_COUNT;
    return arena;
}

cp_temp_arena_t cp_scratch_begin(void) {
    cp_arena_t *a = cp_scratch_get();
    return cp_temp_begin(a);
}

void cp_scratch_end(cp_temp_arena_t temp) {
    cp_temp_end(temp);
}

/* -------------------------------------------------------------------------
 * Fixed-Size Pool Implementation
 * ------------------------------------------------------------------------- */
cp_pool_t *cp_pool_create(usize item_size, usize item_count) {
    if (item_size < sizeof(cp_pool_node_t)) {
        item_size = sizeof(cp_pool_node_t);
    }
    item_size = (usize)_cp_align_up((uintptr_t)item_size, sizeof(void *));

    cp_pool_t *pool = (cp_pool_t *)malloc(sizeof(cp_pool_t));
    if (!pool) Panic("Failed to allocate Pool structure");

    pool->item_size = item_size;
    pool->item_count = item_count;
    pool->capacity = item_size * item_count;
    pool->used = 0;
    pool->buffer = (u8 *)malloc(pool->capacity);
    if (!pool->buffer) {
        free(pool);
        Panic("Failed to allocate Pool storage buffer");
    }

    /* Build free list */
    pool->free_list = (cp_pool_node_t *)pool->buffer;
    cp_pool_node_t *cur = pool->free_list;
    for (usize i = 0; i < item_count - 1; i++) {
        cur->next = (cp_pool_node_t *)((u8 *)cur + item_size);
        cur = cur->next;
    }
    cur->next = NULL;

    return pool;
}

void cp_pool_destroy(cp_pool_t *pool) {
    if (!pool) return;
    if (pool->buffer) free(pool->buffer);
    free(pool);
}

void *cp_pool_alloc(cp_pool_t *pool) {
    if (!pool || !pool->free_list) return NULL;
    cp_pool_node_t *node = pool->free_list;
    pool->free_list = node->next;
    pool->used++;
    return (void *)node;
}

void cp_pool_free(cp_pool_t *pool, void *ptr) {
    if (!pool || !ptr) return;
    cp_pool_node_t *node = (cp_pool_node_t *)ptr;
    node->next = pool->free_list;
    pool->free_list = node;
    if (pool->used > 0) pool->used--;
}

/* -------------------------------------------------------------------------
 * Tracked Heap Allocator Implementation
 * ------------------------------------------------------------------------- */
#define CP_CANARY_VALUE 0xDEADBEEFCAFEBABEU

typedef struct cp_heap_header {
    u64 canary;
    usize size;
    const char *file;
    int line;
    struct cp_heap_header *prev;
    struct cp_heap_header *next;
} cp_heap_header_t;

static cp_heap_header_t *g_heap_head = NULL;
static usize g_heap_active_count = 0;
static pthread_mutex_t g_heap_mutex = PTHREAD_MUTEX_INITIALIZER;

void *cp_heap_alloc_debug(usize size, const char *file, int line) {
    if (size == 0) return NULL;
    usize total = sizeof(cp_heap_header_t) + size + sizeof(u64); /* Pre + Post Canary */
    cp_heap_header_t *hdr = (cp_heap_header_t *)malloc(total);
    if (!hdr) return NULL;

    hdr->canary = CP_CANARY_VALUE;
    hdr->size = size;
    hdr->file = file;
    hdr->line = line;

    /* Write trailing canary */
    u64 *tail_canary = (u64 *)((u8 *)(hdr + 1) + size);
    *tail_canary = CP_CANARY_VALUE;

    pthread_mutex_lock(&g_heap_mutex);
    hdr->prev = NULL;
    hdr->next = g_heap_head;
    if (g_heap_head) {
        g_heap_head->prev = hdr;
    }
    g_heap_head = hdr;
    g_heap_active_count++;
    pthread_mutex_unlock(&g_heap_mutex);

    return (void *)(hdr + 1);
}

void *cp_heap_realloc_debug(void *ptr, usize size, const char *file, int line) {
    if (!ptr) return cp_heap_alloc_debug(size, file, line);
    if (size == 0) {
        cp_heap_free_debug(ptr, file, line);
        return NULL;
    }

    cp_heap_header_t *old_hdr = (cp_heap_header_t *)ptr - 1;
    if (old_hdr->canary != CP_CANARY_VALUE) {
        Panic("Corrupted heap block detected in MemRealloc!");
    }

    void *new_ptr = cp_heap_alloc_debug(size, file, line);
    if (!new_ptr) return NULL;

    usize copy_size = old_hdr->size < size ? old_hdr->size : size;
    memcpy(new_ptr, ptr, copy_size);
    cp_heap_free_debug(ptr, file, line);
    return new_ptr;
}

void cp_heap_free_debug(void *ptr, const char *file, int line) {
    if (!ptr) return;
    cp_heap_header_t *hdr = (cp_heap_header_t *)ptr - 1;

    if (hdr->canary != CP_CANARY_VALUE) {
        fprintf(stderr, "[CPLUS MEMORY ERROR] Corrupted block header or double-free at %s:%d\n", file, line);
        Panic("Memory corruption in MemFree");
    }

    u64 *tail_canary = (u64 *)((u8 *)(hdr + 1) + hdr->size);
    if (*tail_canary != CP_CANARY_VALUE) {
        fprintf(stderr, "[CPLUS BUFFER OVERFLOW] Heap buffer overflow detected! Block allocated at %s:%d, freed at %s:%d\n",
                hdr->file, hdr->line, file, line);
        Panic("Buffer overflow detected in MemFree");
    }

    pthread_mutex_lock(&g_heap_mutex);
    if (hdr->prev) {
        hdr->prev->next = hdr->next;
    } else {
        g_heap_head = hdr->next;
    }
    if (hdr->next) {
        hdr->next->prev = hdr->prev;
    }
    g_heap_active_count--;
    pthread_mutex_unlock(&g_heap_mutex);

    /* Invalidate canary before free */
    hdr->canary = 0;
    free(hdr);
}

usize cp_heap_active_leaks(void) {
    pthread_mutex_lock(&g_heap_mutex);
    usize count = g_heap_active_count;
    pthread_mutex_unlock(&g_heap_mutex);
    return count;
}

void cp_heap_report_leaks(void) {
    pthread_mutex_lock(&g_heap_mutex);
    if (g_heap_active_count == 0) {
        printf("[CPLUS MEMORY REPORT] All heap allocations cleared. 0 leaks detected.\n");
    } else {
        printf("\n==================================================\n");
        printf("[CPLUS MEMORY REPORT] DETECTED %zu UNFREED ALLOCATIONS:\n", g_heap_active_count);
        printf("==================================================\n");
        cp_heap_header_t *cur = g_heap_head;
        usize i = 1;
        while (cur) {
            printf("  #%zu: %zu bytes allocated at %s:%d (addr: %p)\n",
                   i++, cur->size, cur->file, cur->line, (void *)(cur + 1));
            cur = cur->next;
        }
        printf("==================================================\n\n");
    }
    pthread_mutex_unlock(&g_heap_mutex);
}

/* --- Source: cp_str.c --- */
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

/* --- Source: cp_fs.c --- */
/**
 * @file cp_fs.c
 * @brief High-performance implementation of Filesystem and Path operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

/* -------------------------------------------------------------------------
 * Filesystem Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_fs_read(cp_arena_t *arena, const char *path) {
    if (!path) return (cp_strview_t){ NULL, 0 };
    if (!arena) arena = Scratch();

    FILE *f = fopen(path, "rb");
    if (!f) return (cp_strview_t){ NULL, 0 };

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)ArenaAlloc(arena, (usize)size + 1);
    if (!buf) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }

    usize bytes_read = fread(buf, 1, (usize)size, f);
    fclose(f);

    buf[bytes_read] = '\0';
    return cp_sv_from_parts(buf, bytes_read);
}

b32 cp_fs_write(const char *path, const void *data, usize len) {
    if (!path) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    if (len > 0 && data) {
        usize written = fwrite(data, 1, len, f);
        fclose(f);
        return written == len;
    }

    fclose(f);
    return true;
}

b32 cp_fs_append(const char *path, const void *data, usize len) {
    if (!path) return false;
    FILE *f = fopen(path, "ab");
    if (!f) return false;

    if (len > 0 && data) {
        usize written = fwrite(data, 1, len, f);
        fclose(f);
        return written == len;
    }

    fclose(f);
    return true;
}

b32 cp_fs_exists(const char *path) {
    if (!path) return false;
    return access(path, F_OK) == 0;
}

b32 cp_fs_is_dir(const char *path) {
    if (!path) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

usize cp_fs_size(const char *path) {
    if (!path) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (usize)st.st_size;
}

b32 cp_fs_remove(const char *path) {
    if (!path) return false;
    return remove(path) == 0;
}

b32 cp_fs_mkdir(const char *path) {
    if (!path) return false;
    return mkdir(path, 0755) == 0;
}

cp_strview_t *cp_fs_list_dir(cp_arena_t *arena, const char *path, usize *out_count) {
    if (!path) {
        if (out_count) *out_count = 0;
        return NULL;
    }
    if (!arena) arena = Scratch();

    DIR *dir = opendir(path);
    if (!dir) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    /* Count items */
    usize count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        count++;
    }
    rewinddir(dir);

    cp_strview_t *results = (cp_strview_t *)ArenaAlloc(arena, count * sizeof(cp_strview_t));
    usize idx = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        usize name_len = strlen(entry->d_name);
        char *copy = (char *)ArenaAlloc(arena, name_len + 1);
        memcpy(copy, entry->d_name, name_len);
        copy[name_len] = '\0';
        results[idx++] = cp_sv_from_parts(copy, name_len);
    }
    closedir(dir);

    if (out_count) *out_count = count;
    return results;
}

/* -------------------------------------------------------------------------
 * Path Manipulation Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_path_join(cp_arena_t *arena, cp_strview_t a, cp_strview_t b) {
    if (!arena) arena = Scratch();

    while (a.len > 0 && a.data[a.len - 1] == '/') {
        a.len--;
    }
    while (b.len > 0 && b.data[0] == '/') {
        b.data++;
        b.len--;
    }

    usize total_len = a.len + 1 + b.len;
    char *buf = (char *)ArenaAlloc(arena, total_len + 1);
    memcpy(buf, a.data, a.len);
    buf[a.len] = '/';
    memcpy(buf + a.len + 1, b.data, b.len);
    buf[total_len] = '\0';

    return cp_sv_from_parts(buf, total_len);
}

cp_strview_t cp_path_ext(cp_strview_t path) {
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '.') {
            return cp_sv_sub(path, i - 1, path.len - (i - 1));
        }
        if (path.data[i - 1] == '/') {
            break;
        }
    }
    return (cp_strview_t){ NULL, 0 };
}

cp_strview_t cp_path_base(cp_strview_t path) {
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '/') {
            return cp_sv_sub(path, i, path.len - i);
        }
    }
    return path;
}

cp_strview_t cp_path_dir(cp_arena_t *arena, cp_strview_t path) {
    if (!arena) arena = Scratch();
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '/') {
            return cp_sv_sub(path, 0, i - 1);
        }
    }
    return cp_sv_from_c(".");
}

/* --- Source: cp_ds.c --- */
/**
 * @file cp_ds.c
 * @brief High-performance implementation of Vector, Singly/Doubly Linked Lists,
 *        and Lock-Free SPSC RingBuffer.
 */

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Dynamic Vector Implementation
 * ------------------------------------------------------------------------- */
void *_cp_vec_grow(void *vec, usize elem_size, usize add_len, usize min_cap) {
    usize old_cap = cp_vec_cap(vec);
    usize old_len = cp_vec_len(vec);
    usize new_cap = old_cap ? old_cap * 2 : min_cap;
    if (new_cap < old_len + add_len) {
        new_cap = old_len + add_len;
    }

    usize total_bytes = sizeof(cp_vec_header_t) + new_cap * elem_size;
    cp_vec_header_t *hdr = NULL;

    if (vec) {
        hdr = (cp_vec_header_t *)realloc(cp_vec_header(vec), total_bytes);
    } else {
        hdr = (cp_vec_header_t *)malloc(total_bytes);
        if (hdr) hdr->len = 0;
    }

    if (!hdr) {
        Panic("Out of memory growing dynamic Vector");
    }

    hdr->cap = new_cap;
    return (void *)(hdr + 1);
}

void _cp_vec_free(void *vec) {
    if (!vec) return;
    free(cp_vec_header(vec));
}

/* -------------------------------------------------------------------------
 * Singly Linked List Implementation
 * ------------------------------------------------------------------------- */
cp_slist_t cp_slist_new(cp_arena_t *arena) {
    cp_slist_t l;
    l.head = NULL;
    l.tail = NULL;
    l.len = 0;
    l.arena = arena;
    return l;
}

static cp_snode_t *_cp_snode_alloc(cp_slist_t *list, void *val) {
    cp_snode_t *node = NULL;
    if (list->arena) {
        node = (cp_snode_t *)ArenaAlloc(list->arena, sizeof(cp_snode_t));
    } else {
        node = (cp_snode_t *)malloc(sizeof(cp_snode_t));
    }
    if (!node) Panic("Failed to allocate Singly Linked List node");
    node->val = val;
    node->next = NULL;
    return node;
}

void cp_slist_push_front(cp_slist_t *list, void *val) {
    if (!list) return;
    cp_snode_t *node = _cp_snode_alloc(list, val);
    node->next = list->head;
    list->head = node;
    if (!list->tail) {
        list->tail = node;
    }
    list->len++;
}

void cp_slist_push_back(cp_slist_t *list, void *val) {
    if (!list) return;
    cp_snode_t *node = _cp_snode_alloc(list, val);
    if (list->tail) {
        list->tail->next = node;
        list->tail = node;
    } else {
        list->head = node;
        list->tail = node;
    }
    list->len++;
}

void *cp_slist_pop_front(cp_slist_t *list) {
    if (!list || !list->head) return NULL;
    cp_snode_t *node = list->head;
    void *val = node->val;
    list->head = node->next;
    if (!list->head) {
        list->tail = NULL;
    }
    list->len--;
    if (!list->arena) {
        free(node);
    }
    return val;
}

void cp_slist_clear(cp_slist_t *list) {
    if (!list) return;
    if (!list->arena) {
        cp_snode_t *cur = list->head;
        while (cur) {
            cp_snode_t *next = cur->next;
            free(cur);
            cur = next;
        }
    }
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
}

/* -------------------------------------------------------------------------
 * Doubly Linked List Implementation
 * ------------------------------------------------------------------------- */
cp_dlist_t cp_dlist_new(cp_arena_t *arena) {
    cp_dlist_t dl;
    dl.head = NULL;
    dl.tail = NULL;
    dl.len = 0;
    dl.arena = arena;
    return dl;
}

static cp_dnode_t *_cp_dnode_alloc(cp_dlist_t *list, void *val) {
    cp_dnode_t *node = NULL;
    if (list->arena) {
        node = (cp_dnode_t *)ArenaAlloc(list->arena, sizeof(cp_dnode_t));
    } else {
        node = (cp_dnode_t *)malloc(sizeof(cp_dnode_t));
    }
    if (!node) Panic("Failed to allocate Doubly Linked List node");
    node->val = val;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

cp_dnode_t *cp_dlist_push_front(cp_dlist_t *list, void *val) {
    if (!list) return NULL;
    cp_dnode_t *node = _cp_dnode_alloc(list, val);
    node->next = list->head;
    if (list->head) {
        list->head->prev = node;
    }
    list->head = node;
    if (!list->tail) {
        list->tail = node;
    }
    list->len++;
    return node;
}

cp_dnode_t *cp_dlist_push_back(cp_dlist_t *list, void *val) {
    if (!list) return NULL;
    cp_dnode_t *node = _cp_dnode_alloc(list, val);
    node->prev = list->tail;
    if (list->tail) {
        list->tail->next = node;
    }
    list->tail = node;
    if (!list->head) {
        list->head = node;
    }
    list->len++;
    return node;
}

void *cp_dlist_pop_front(cp_dlist_t *list) {
    if (!list || !list->head) return NULL;
    cp_dnode_t *node = list->head;
    void *val = node->val;
    list->head = node->next;
    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }
    list->len--;
    if (!list->arena) free(node);
    return val;
}

void *cp_dlist_pop_back(cp_dlist_t *list) {
    if (!list || !list->tail) return NULL;
    cp_dnode_t *node = list->tail;
    void *val = node->val;
    list->tail = node->prev;
    if (list->tail) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }
    list->len--;
    if (!list->arena) free(node);
    return val;
}

void cp_dlist_remove(cp_dlist_t *list, cp_dnode_t *node) {
    if (!list || !node) return;

    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }

    list->len--;
    if (!list->arena) free(node);
}

cp_dnode_t *cp_dlist_insert_after(cp_dlist_t *list, cp_dnode_t *node, void *val) {
    if (!list || !node) return NULL;
    cp_dnode_t *new_node = _cp_dnode_alloc(list, val);
    new_node->prev = node;
    new_node->next = node->next;

    if (node->next) {
        node->next->prev = new_node;
    } else {
        list->tail = new_node;
    }
    node->next = new_node;
    list->len++;
    return new_node;
}

void cp_dlist_clear(cp_dlist_t *list) {
    if (!list) return;
    if (!list->arena) {
        cp_dnode_t *cur = list->head;
        while (cur) {
            cp_dnode_t *next = cur->next;
            free(cur);
            cur = next;
        }
    }
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
}

/* -------------------------------------------------------------------------
 * Lock-Free SPSC RingBuffer Implementation
 * ------------------------------------------------------------------------- */
cp_ringbuf_t *cp_ringbuf_create(usize capacity_power_of_two) {
    if ((capacity_power_of_two & (capacity_power_of_two - 1)) != 0 || capacity_power_of_two < 2) {
        Panic("RingBuffer capacity must be a power of two (>= 2)");
    }

    cp_ringbuf_t *rb = (cp_ringbuf_t *)malloc(sizeof(cp_ringbuf_t));
    if (!rb) Panic("Failed to allocate RingBuffer structure");

    rb->capacity = capacity_power_of_two;
    rb->mask = capacity_power_of_two - 1;
    rb->buffer = (void **)malloc(capacity_power_of_two * sizeof(void *));
    if (!rb->buffer) {
        free(rb);
        Panic("Failed to allocate RingBuffer storage");
    }

    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
    return rb;
}

void cp_ringbuf_destroy(cp_ringbuf_t *rb) {
    if (!rb) return;
    if (rb->buffer) free(rb->buffer);
    free(rb);
}

b32 cp_ringbuf_push(cp_ringbuf_t *rb, void *item) {
    if (!rb) return false;

    usize current_tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    usize current_head = atomic_load_explicit(&rb->head, memory_order_acquire);

    if (current_tail - current_head >= rb->capacity) {
        return false; /* Buffer full */
    }

    rb->buffer[current_tail & rb->mask] = item;
    atomic_store_explicit(&rb->tail, current_tail + 1, memory_order_release);
    return true;
}

b32 cp_ringbuf_pop(cp_ringbuf_t *rb, void **out_item) {
    if (!rb || !out_item) return false;

    usize current_head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    usize current_tail = atomic_load_explicit(&rb->tail, memory_order_acquire);

    if (current_head == current_tail) {
        return false; /* Buffer empty */
    }

    *out_item = rb->buffer[current_head & rb->mask];
    atomic_store_explicit(&rb->head, current_head + 1, memory_order_release);
    return true;
}

usize cp_ringbuf_count(const cp_ringbuf_t *rb) {
    if (!rb) return 0;
    usize current_head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    usize current_tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    return current_tail >= current_head ? current_tail - current_head : 0;
}

/* --- Source: cp_graph.c --- */
/**
 * @file cp_graph.c
 * @brief High-performance Graph implementation with Adjacency List and Min-Heap Dijkstra.
 */

#include <stdlib.h>
#include <string.h>
#include <float.h>

/* -------------------------------------------------------------------------
 * Graph Creation & Edge Operations
 * ------------------------------------------------------------------------- */
cp_graph_t cp_graph_new(cp_arena_t *arena, usize node_count, b32 is_directed) {
    if (!arena) arena = Scratch();

    cp_graph_t g;
    g.node_count = node_count;
    g.edge_count = 0;
    g.is_directed = is_directed;
    g.arena = arena;
    g.adj = (cp_graph_edge_t **)ArenaAllocZero(arena, node_count * sizeof(cp_graph_edge_t *));
    return g;
}

static cp_graph_edge_t *_cp_alloc_edge(cp_arena_t *arena, usize to, f64 weight, cp_graph_edge_t *next) {
    cp_graph_edge_t *e = (cp_graph_edge_t *)ArenaAlloc(arena, sizeof(cp_graph_edge_t));
    e->to = to;
    e->weight = weight;
    e->next = next;
    return e;
}

void cp_graph_add_edge(cp_graph_t *g, usize from, usize to, f64 weight) {
    if (!g || from >= g->node_count || to >= g->node_count) return;

    g->adj[from] = _cp_alloc_edge(g->arena, to, weight, g->adj[from]);
    if (!g->is_directed && from != to) {
        g->adj[to] = _cp_alloc_edge(g->arena, from, weight, g->adj[to]);
    }
    g->edge_count++;
}

usize cp_graph_node_count(const cp_graph_t *g) {
    return g ? g->node_count : 0;
}

usize cp_graph_edge_count(const cp_graph_t *g) {
    return g ? g->edge_count : 0;
}

/* -------------------------------------------------------------------------
 * Graph Traversals (BFS & DFS)
 * ------------------------------------------------------------------------- */
void cp_graph_bfs(const cp_graph_t *g, usize start_node, cp_graph_visitor_fn visitor, void *user_data) {
    if (!g || start_node >= g->node_count || !visitor) return;

    cp_temp_arena_t scratch = ScratchBegin();
    b8 *visited = (b8 *)ArenaAllocZero(scratch.arena, g->node_count * sizeof(b8));
    usize *queue = (usize *)ArenaAlloc(scratch.arena, g->node_count * sizeof(usize));

    usize q_head = 0;
    usize q_tail = 0;

    visited[start_node] = 1;
    queue[q_tail++] = start_node;

    while (q_head < q_tail) {
        usize u = queue[q_head++];
        visitor(u, user_data);

        for (cp_graph_edge_t *e = g->adj[u]; e != NULL; e = e->next) {
            if (!visited[e->to]) {
                visited[e->to] = 1;
                queue[q_tail++] = e->to;
            }
        }
    }

    ScratchEnd(scratch);
}

void cp_graph_dfs(const cp_graph_t *g, usize start_node, cp_graph_visitor_fn visitor, void *user_data) {
    if (!g || start_node >= g->node_count || !visitor) return;

    cp_temp_arena_t scratch = ScratchBegin();
    b8 *visited = (b8 *)ArenaAllocZero(scratch.arena, g->node_count * sizeof(b8));
    usize *stack = (usize *)ArenaAlloc(scratch.arena, g->node_count * sizeof(usize));

    usize stack_top = 0;
    stack[stack_top++] = start_node;

    while (stack_top > 0) {
        usize u = stack[--stack_top];
        if (visited[u]) continue;

        visited[u] = 1;
        visitor(u, user_data);

        /* Push neighbors in reverse to maintain left-to-right visit order */
        for (cp_graph_edge_t *e = g->adj[u]; e != NULL; e = e->next) {
            if (!visited[e->to]) {
                stack[stack_top++] = e->to;
            }
        }
    }

    ScratchEnd(scratch);
}

/* -------------------------------------------------------------------------
 * Min-Heap Priority Queue for Dijkstra
 * ------------------------------------------------------------------------- */
typedef struct {
    usize node;
    f64 dist;
} cp_heap_entry_t;

typedef struct {
    cp_heap_entry_t *data;
    usize size;
    usize capacity;
} cp_min_heap_t;

static cp_min_heap_t _cp_heap_create(cp_arena_t *arena, usize capacity) {
    cp_min_heap_t h;
    h.data = (cp_heap_entry_t *)ArenaAlloc(arena, capacity * sizeof(cp_heap_entry_t));
    h.size = 0;
    h.capacity = capacity;
    return h;
}

static void _cp_heap_push(cp_min_heap_t *h, usize node, f64 dist) {
    if (h->size >= h->capacity) return;

    usize i = h->size++;
    h->data[i] = (cp_heap_entry_t){ node, dist };

    while (i > 0) {
        usize parent = (i - 1) / 2;
        if (h->data[parent].dist <= h->data[i].dist) break;

        cp_heap_entry_t temp = h->data[parent];
        h->data[parent] = h->data[i];
        h->data[i] = temp;
        i = parent;
    }
}

static b32 _cp_heap_pop(cp_min_heap_t *h, cp_heap_entry_t *out_entry) {
    if (h->size == 0) return false;

    *out_entry = h->data[0];
    h->data[0] = h->data[--h->size];

    usize i = 0;
    while (2 * i + 1 < h->size) {
        usize left = 2 * i + 1;
        usize right = 2 * i + 2;
        usize smallest = left;

        if (right < h->size && h->data[right].dist < h->data[left].dist) {
            smallest = right;
        }

        if (h->data[i].dist <= h->data[smallest].dist) break;

        cp_heap_entry_t temp = h->data[i];
        h->data[i] = h->data[smallest];
        h->data[smallest] = temp;
        i = smallest;
    }

    return true;
}

/* -------------------------------------------------------------------------
 * Dijkstra Shortest Path Algorithm Implementation
 * ------------------------------------------------------------------------- */
b32 cp_graph_dijkstra(const cp_graph_t *g, usize start, usize target, f64 *out_dist, Vec(usize) *out_path) {
    if (!g || start >= g->node_count || target >= g->node_count) return false;

    cp_temp_arena_t scratch = ScratchBegin();

    f64 *dist = (f64 *)ArenaAlloc(scratch.arena, g->node_count * sizeof(f64));
    usize *prev = (usize *)ArenaAlloc(scratch.arena, g->node_count * sizeof(usize));

    for (usize i = 0; i < g->node_count; i++) {
        dist[i] = DBL_MAX;
        prev[i] = (usize)-1;
    }

    dist[start] = 0.0;
    usize heap_cap = (g->edge_count > 0 ? g->edge_count * 2 : 16) + 16;
    cp_min_heap_t heap = _cp_heap_create(scratch.arena, heap_cap);
    _cp_heap_push(&heap, start, 0.0);

    cp_heap_entry_t current;
    while (_cp_heap_pop(&heap, &current)) {
        usize u = current.node;
        f64 d = current.dist;

        if (d > dist[u]) continue;
        if (u == target) break; /* Reached target node */

        for (cp_graph_edge_t *e = g->adj[u]; e != NULL; e = e->next) {
            usize v = e->to;
            f64 new_dist = dist[u] + e->weight;

            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                prev[v] = u;
                _cp_heap_push(&heap, v, new_dist);
            }
        }
    }

    if (dist[target] == DBL_MAX) {
        ScratchEnd(scratch);
        return false; /* No path exists */
    }

    if (out_dist) {
        *out_dist = dist[target];
    }

    if (out_path) {
        /* Reconstruct path backwards */
        Vec(usize) path = VecNew(usize);
        usize curr = target;
        while (curr != (usize)-1) {
            VecPush(path, curr);
            curr = prev[curr];
        }

        /* Reverse path */
        usize len = VecLen(path);
        for (usize i = 0; i < len / 2; i++) {
            usize tmp = path[i];
            path[i] = path[len - 1 - i];
            path[len - 1 - i] = tmp;
        }
        *out_path = path;
    }

    ScratchEnd(scratch);
    return true;
}

/* --- Source: cp_algo.c --- */
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

/* --- Source: cp_math.c --- */
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

/* --- Source: cp_thread.c --- */
/**
 * @file cp_thread.c
 * @brief Implementation of low-latency worker thread pool and threading utilities.
 */

#include <unistd.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Condition Variable Timed Wait
 * ------------------------------------------------------------------------- */
bool cp_cond_timedwait_ms(cp_cond_t *c, cp_mutex_t *m, u32 timeout_ms) {
    if (!c || !m) return false;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return pthread_cond_timedwait(c, m, &ts) == 0;
}

/* -------------------------------------------------------------------------
 * Low-Latency Thread Pool Implementation
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*fn)(void *arg);
    void *arg;
} cp_pool_task_t;

struct cp_threadpool {
    cp_thread_t *threads;
    usize num_threads;
    cp_pool_task_t *queue;
    usize queue_capacity;
    usize queue_head;
    usize queue_tail;
    usize queue_count;
    usize active_workers;
    cp_mutex_t lock;
    cp_cond_t not_empty;
    cp_cond_t not_full;
    cp_cond_t all_idle;
    bool shutdown;
};

static void *cp_threadpool_worker(void *arg) {
    cp_threadpool_t *pool = (cp_threadpool_t *)arg;
    while (1) {
        cp_mutex_lock(&pool->lock);
        while (pool->queue_count == 0 && !pool->shutdown) {
            cp_cond_wait(&pool->not_empty, &pool->lock);
        }
        if (pool->shutdown && pool->queue_count == 0) {
            cp_mutex_unlock(&pool->lock);
            break;
        }

        cp_pool_task_t task = pool->queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % pool->queue_capacity;
        pool->queue_count--;
        pool->active_workers++;
        cp_cond_signal(&pool->not_full);
        cp_mutex_unlock(&pool->lock);

        if (task.fn) {
            task.fn(task.arg);
        }

        cp_mutex_lock(&pool->lock);
        pool->active_workers--;
        if (pool->queue_count == 0 && pool->active_workers == 0) {
            cp_cond_broadcast(&pool->all_idle);
        }
        cp_mutex_unlock(&pool->lock);
    }
    return NULL;
}

cp_threadpool_t *cp_threadpool_create(usize num_threads, usize queue_capacity) {
    if (num_threads == 0) {
        long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
        num_threads = (nprocs > 0) ? (usize)nprocs : 4;
    }
    if (queue_capacity == 0) {
        queue_capacity = 4096;
    }

    cp_threadpool_t *pool = (cp_threadpool_t *)calloc(1, sizeof(cp_threadpool_t));
    if (!pool) return NULL;

    pool->num_threads = num_threads;
    pool->queue_capacity = queue_capacity;
    pool->queue = (cp_pool_task_t *)calloc(queue_capacity, sizeof(cp_pool_task_t));
    pool->threads = (cp_thread_t *)calloc(num_threads, sizeof(cp_thread_t));
    if (!pool->queue || !pool->threads) {
        free(pool->queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    cp_mutex_init(&pool->lock);
    cp_cond_init(&pool->not_empty);
    cp_cond_init(&pool->not_full);
    cp_cond_init(&pool->all_idle);

    for (usize i = 0; i < num_threads; ++i) {
        if (pthread_create(&pool->threads[i], NULL, cp_threadpool_worker, pool) != 0) {
            pool->shutdown = true;
            cp_threadpool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

bool cp_threadpool_submit(cp_threadpool_t *pool, void (*fn)(void *), void *arg) {
    if (!pool || !fn) return false;
    cp_mutex_lock(&pool->lock);
    while (pool->queue_count == pool->queue_capacity && !pool->shutdown) {
        cp_cond_wait(&pool->not_full, &pool->lock);
    }
    if (pool->shutdown) {
        cp_mutex_unlock(&pool->lock);
        return false;
    }

    pool->queue[pool->queue_tail].fn = fn;
    pool->queue[pool->queue_tail].arg = arg;
    pool->queue_tail = (pool->queue_tail + 1) % pool->queue_capacity;
    pool->queue_count++;
    cp_cond_signal(&pool->not_empty);
    cp_mutex_unlock(&pool->lock);
    return true;
}

bool cp_threadpool_try_submit(cp_threadpool_t *pool, void (*fn)(void *), void *arg) {
    if (!pool || !fn) return false;
    cp_mutex_lock(&pool->lock);
    if (pool->queue_count == pool->queue_capacity || pool->shutdown) {
        cp_mutex_unlock(&pool->lock);
        return false;
    }

    pool->queue[pool->queue_tail].fn = fn;
    pool->queue[pool->queue_tail].arg = arg;
    pool->queue_tail = (pool->queue_tail + 1) % pool->queue_capacity;
    pool->queue_count++;
    cp_cond_signal(&pool->not_empty);
    cp_mutex_unlock(&pool->lock);
    return true;
}

void cp_threadpool_wait(cp_threadpool_t *pool) {
    if (!pool) return;
    cp_mutex_lock(&pool->lock);
    while ((pool->queue_count > 0 || pool->active_workers > 0) && !pool->shutdown) {
        cp_cond_wait(&pool->all_idle, &pool->lock);
    }
    cp_mutex_unlock(&pool->lock);
}

void cp_threadpool_destroy(cp_threadpool_t *pool) {
    if (!pool) return;
    cp_mutex_lock(&pool->lock);
    pool->shutdown = true;
    cp_cond_broadcast(&pool->not_empty);
    cp_cond_broadcast(&pool->not_full);
    cp_mutex_unlock(&pool->lock);

    for (usize i = 0; i < pool->num_threads; ++i) {
        pthread_join(pool->threads[i], NULL);
    }

    cp_mutex_destroy(&pool->lock);
    cp_cond_destroy(&pool->not_empty);
    cp_cond_destroy(&pool->not_full);
    cp_cond_destroy(&pool->all_idle);

    free(pool->threads);
    free(pool->queue);
    free(pool);
}

usize cp_threadpool_num_threads(const cp_threadpool_t *pool) {
    return pool ? pool->num_threads : 1;
}

/* -------------------------------------------------------------------------
 * Parallel-For Chunk Execution Engine
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_parallel_for_fn fn;
    usize start;
    usize end;
    void *arg;
    usize *remaining;
    cp_mutex_t *lock;
    cp_cond_t *cond;
} cp_parallel_chunk_t;

static void _cp_parallel_worker_task(void *raw) {
    cp_parallel_chunk_t *chunk = (cp_parallel_chunk_t *)raw;
    chunk->fn(chunk->start, chunk->end, chunk->arg);
    cp_mutex_lock(chunk->lock);
    (*chunk->remaining)--;
    if (*chunk->remaining == 0) {
        cp_cond_signal(chunk->cond);
    }
    cp_mutex_unlock(chunk->lock);
}

void cp_threadpool_parallel_for(cp_threadpool_t *pool, usize count, cp_parallel_for_fn fn, void *arg) {
    if (count == 0 || !fn) return;
    if (!pool || pool->num_threads <= 1 || count == 1) {
        fn(0, count, arg);
        return;
    }

    usize k = pool->num_threads;
    if (k > count) k = count;
    if (k > 128) k = 128;

    cp_parallel_chunk_t chunks[128];
    usize remaining = k;
    cp_mutex_t lock;
    cp_mutex_init(&lock);
    cp_cond_t cond;
    cp_cond_init(&cond);

    usize base_chunk = count / k;
    usize rem = count % k;
    usize cur_start = 0;

    for (usize i = 0; i < k; ++i) {
        usize slice = base_chunk + (i < rem ? 1 : 0);
        chunks[i].fn = fn;
        chunks[i].start = cur_start;
        chunks[i].end = cur_start + slice;
        chunks[i].arg = arg;
        chunks[i].remaining = &remaining;
        chunks[i].lock = &lock;
        chunks[i].cond = &cond;
        cur_start += slice;

        if (i < k - 1) {
            cp_threadpool_submit(pool, _cp_parallel_worker_task, &chunks[i]);
        }
    }

    /* Current calling thread processes chunk k - 1 directly */
    chunks[k - 1].fn(chunks[k - 1].start, chunks[k - 1].end, chunks[k - 1].arg);

    cp_mutex_lock(&lock);
    remaining--;
    while (remaining > 0) {
        cp_cond_wait(&cond, &lock);
    }
    cp_mutex_unlock(&lock);

    cp_mutex_destroy(&lock);
    cp_cond_destroy(&cond);
}

/* --- Source: cp_net.c --- */
/**
 * @file cp_net.c
 * @brief High-performance networking primitives, non-blocking TCP/UDP, Modbus RTU/TCP, and SocketCAN.
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

/* -------------------------------------------------------------------------
 * Socket Descriptors & Configuration
 * ------------------------------------------------------------------------- */
void cp_net_close(cp_socket_t fd) {
    if (fd >= 0) {
        close(fd);
    }
}

bool cp_net_set_nonblocking(cp_socket_t fd, bool nonblocking) {
    if (fd < 0) return false;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    if (nonblocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    return fcntl(fd, F_SETFL, flags) != -1;
}

bool cp_net_set_reuseaddr(cp_socket_t fd, bool reuse) {
    if (fd < 0) return false;
    int opt = reuse ? 1 : 0;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}

bool cp_net_set_tcp_nodelay(cp_socket_t fd, bool nodelay) {
    if (fd < 0) return false;
    int opt = nodelay ? 1 : 0;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == 0;
}

/* -------------------------------------------------------------------------
 * TCP Sockets
 * ------------------------------------------------------------------------- */
cp_socket_t cp_net_tcp_connect(const char *host, u16 port, u32 timeout_ms) {
    if (!host) return CP_INVALID_SOCKET;
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    struct addrinfo hints, *res = NULL, *p = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* IPv4 */
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        return CP_INVALID_SOCKET;
    }

    cp_socket_t fd = CP_INVALID_SOCKET;
    for (p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;

        cp_net_set_tcp_nodelay(fd, true);

        if (timeout_ms > 0) {
            cp_net_set_nonblocking(fd, true);
            int ret = connect(fd, p->ai_addr, p->ai_addrlen);
            if (ret == 0) {
                /* Connected immediately */
                cp_net_set_nonblocking(fd, false);
                break;
            }
            if (errno == EINPROGRESS) {
                struct pollfd pfd = { .fd = fd, .events = POLLOUT, .revents = 0 };
                int pr = poll(&pfd, 1, (int)timeout_ms);
                if (pr > 0 && (pfd.revents & POLLOUT)) {
                    int err = 0;
                    socklen_t elen = sizeof(err);
                    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen) == 0 && err == 0) {
                        cp_net_set_nonblocking(fd, false);
                        break; /* Success */
                    }
                }
            }
            close(fd);
            fd = CP_INVALID_SOCKET;
        } else {
            if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
                break; /* Success */
            }
            close(fd);
            fd = CP_INVALID_SOCKET;
        }
    }

    freeaddrinfo(res);
    return fd;
}

cp_socket_t cp_net_tcp_listen(const char *bind_addr, u16 port, i32 backlog) {
    cp_socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return CP_INVALID_SOCKET;

    cp_net_set_reuseaddr(fd, true);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (!bind_addr || bind_addr[0] == '\0' || strcmp(bind_addr, "0.0.0.0") == 0) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, bind_addr, &sin.sin_addr) <= 0) {
            close(fd);
            return CP_INVALID_SOCKET;
        }
    }

    if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }

    if (listen(fd, (backlog > 0) ? backlog : 128) != 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }

    return fd;
}

cp_socket_t cp_net_tcp_accept(cp_socket_t listen_fd, char *client_ip_out, usize ip_len, u16 *client_port_out) {
    if (listen_fd < 0) return CP_INVALID_SOCKET;
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    cp_socket_t client = accept(listen_fd, (struct sockaddr *)&sin, &slen);
    if (client < 0) return CP_INVALID_SOCKET;

    cp_net_set_tcp_nodelay(client, true);

    if (client_ip_out && ip_len > 0) {
        inet_ntop(AF_INET, &sin.sin_addr, client_ip_out, (socklen_t)ip_len);
    }
    if (client_port_out) {
        *client_port_out = ntohs(sin.sin_port);
    }
    return client;
}

i64 cp_net_send_all(cp_socket_t fd, const void *buf, usize len, u32 timeout_ms) {
    if (fd < 0 || !buf) return -1;
    usize sent = 0;
    const u8 *ptr = (const u8 *)buf;
    while (sent < len) {
        if (timeout_ms > 0) {
            struct pollfd pfd = { .fd = fd, .events = POLLOUT, .revents = 0 };
            int r = poll(&pfd, 1, (int)timeout_ms);
            if (r <= 0) return (sent > 0) ? (i64)sent : -1;
        }
        ssize_t n = send(fd, ptr + sent, len - sent, MSG_NOSIGNAL);
        if (n > 0) {
            sent += (usize)n;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            struct pollfd pfd = { .fd = fd, .events = POLLOUT, .revents = 0 };
            int r = poll(&pfd, 1, (timeout_ms > 0) ? (int)timeout_ms : 5000);
            if (r <= 0) return (sent > 0) ? (i64)sent : -1;
        } else {
            return (sent > 0) ? (i64)sent : -1;
        }
    }
    return (i64)sent;
}

i64 cp_net_recv(cp_socket_t fd, void *buf, usize cap, u32 timeout_ms) {
    if (fd < 0 || !buf || cap == 0) return -1;
    if (timeout_ms > 0) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
        int r = poll(&pfd, 1, (int)timeout_ms);
        if (r <= 0) return -1;
    }
    ssize_t n = recv(fd, buf, cap, 0);
    return (i64)n;
}

bool cp_net_tcp_request(const char *host, u16 port, const void *req, usize req_len,
                        void *resp_buf, usize resp_buf_cap, usize *resp_len_out, u32 timeout_ms) {
    cp_socket_t fd = cp_net_tcp_connect(host, port, timeout_ms);
    if (fd < 0) return false;

    if (req && req_len > 0) {
        if (cp_net_send_all(fd, req, req_len, timeout_ms) < (i64)req_len) {
            cp_net_close(fd);
            return false;
        }
    }

    usize total_recv = 0;
    if (resp_buf && resp_buf_cap > 0) {
        i64 n = cp_net_recv(fd, resp_buf, resp_buf_cap, timeout_ms);
        if (n > 0) {
            total_recv = (usize)n;
        }
    }
    if (resp_len_out) {
        *resp_len_out = total_recv;
    }
    cp_net_close(fd);
    return true;
}

/* -------------------------------------------------------------------------
 * UDP Sockets
 * ------------------------------------------------------------------------- */
cp_socket_t cp_net_udp_bind(const char *bind_addr, u16 port) {
    cp_socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return CP_INVALID_SOCKET;
    cp_net_set_reuseaddr(fd, true);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (!bind_addr || bind_addr[0] == '\0' || strcmp(bind_addr, "0.0.0.0") == 0) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, bind_addr, &sin.sin_addr) <= 0) {
            close(fd);
            return CP_INVALID_SOCKET;
        }
    }

    if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }
    return fd;
}

i64 cp_net_udp_send_to(cp_socket_t fd, const char *host, u16 port, const void *data, usize len) {
    if (fd < 0 || !host || !data) return -1;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &sin.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(host);
        if (!he) return -1;
        memcpy(&sin.sin_addr, he->h_addr_list[0], sizeof(sin.sin_addr));
    }
    ssize_t n = sendto(fd, data, len, 0, (struct sockaddr *)&sin, sizeof(sin));
    return (i64)n;
}

i64 cp_net_udp_recv_from(cp_socket_t fd, void *buf, usize cap, char *sender_ip, usize ip_len, u16 *sender_port, u32 timeout_ms) {
    if (fd < 0 || !buf || cap == 0) return -1;
    if (timeout_ms > 0) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
        int r = poll(&pfd, 1, (int)timeout_ms);
        if (r <= 0) return -1;
    }
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    ssize_t n = recvfrom(fd, buf, cap, 0, (struct sockaddr *)&sin, &slen);
    if (n >= 0) {
        if (sender_ip && ip_len > 0) {
            inet_ntop(AF_INET, &sin.sin_addr, sender_ip, (socklen_t)ip_len);
        }
        if (sender_port) {
            *sender_port = ntohs(sin.sin_port);
        }
    }
    return (i64)n;
}

bool cp_net_udp_send(const char *host, u16 port, const void *data, usize len) {
    cp_socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;
    i64 n = cp_net_udp_send_to(fd, host, port, data, len);
    close(fd);
    return n == (i64)len;
}

/* -------------------------------------------------------------------------
 * Industrial Protocols: Modbus
 * ------------------------------------------------------------------------- */
static const u16 cp_modbus_crc_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

u16 cp_modbus_crc16(const u8 *buf, usize len) {
    if (!buf || len == 0) return 0xFFFF;
    u16 crc = 0xFFFF;
    for (usize i = 0; i < len; ++i) {
        u8 idx = (u8)(crc ^ buf[i]);
        crc = (crc >> 8) ^ cp_modbus_crc_table[idx];
    }
    return crc;
}

usize cp_modbus_rtu_build_read_holding_registers(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count) {
    if (!out || cap < 8) return 0;
    out[0] = slave_id;
    out[1] = 0x03; /* Function code: Read Holding Registers */
    out[2] = (u8)(start_addr >> 8);
    out[3] = (u8)(start_addr & 0xFF);
    out[4] = (u8)(count >> 8);
    out[5] = (u8)(count & 0xFF);
    u16 crc = cp_modbus_crc16(out, 6);
    out[6] = (u8)(crc & 0xFF);        /* LSB first */
    out[7] = (u8)((crc >> 8) & 0xFF); /* MSB */
    return 8;
}

usize cp_modbus_rtu_build_write_single_register(u8 *out, usize cap, u8 slave_id, u16 reg_addr, u16 value) {
    if (!out || cap < 8) return 0;
    out[0] = slave_id;
    out[1] = 0x06; /* Function code: Write Single Register */
    out[2] = (u8)(reg_addr >> 8);
    out[3] = (u8)(reg_addr & 0xFF);
    out[4] = (u8)(value >> 8);
    out[5] = (u8)(value & 0xFF);
    u16 crc = cp_modbus_crc16(out, 6);
    out[6] = (u8)(crc & 0xFF);
    out[7] = (u8)((crc >> 8) & 0xFF);
    return 8;
}

usize cp_modbus_rtu_build_write_multiple_registers(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count, const u16 *values) {
    usize required = 9 + (usize)count * 2;
    if (!out || cap < required || !values) return 0;

    out[0] = slave_id;
    out[1] = 0x10; /* Function code: Write Multiple Registers */
    out[2] = (u8)(start_addr >> 8);
    out[3] = (u8)(start_addr & 0xFF);
    out[4] = (u8)(count >> 8);
    out[5] = (u8)(count & 0xFF);
    out[6] = (u8)(count * 2); /* Byte count */

    for (usize i = 0; i < count; ++i) {
        out[7 + i * 2] = (u8)(values[i] >> 8);
        out[8 + i * 2] = (u8)(values[i] & 0xFF);
    }

    usize pdu_len = 7 + (usize)count * 2;
    u16 crc = cp_modbus_crc16(out, pdu_len);
    out[pdu_len]     = (u8)(crc & 0xFF);
    out[pdu_len + 1] = (u8)((crc >> 8) & 0xFF);
    return required;
}

bool cp_modbus_rtu_verify_crc(const u8 *frame, usize len) {
    if (!frame || len < 4) return false;
    u16 calculated = cp_modbus_crc16(frame, len - 2);
    u16 received = (u16)frame[len - 2] | ((u16)frame[len - 1] << 8);
    return calculated == received;
}

bool cp_modbus_rtu_parse_read_response(const u8 *frame, usize len, u8 expected_slave, u16 *out_regs, u16 max_regs, u16 *out_reg_count) {
    if (!frame || len < 5 || !cp_modbus_rtu_verify_crc(frame, len)) return false;
    if (expected_slave != 0 && frame[0] != expected_slave) return false;
    if (frame[1] != 0x03) return false;

    u8 byte_count = frame[2];
    if (len != (usize)(3 + byte_count + 2)) return false;

    u16 total_regs = (u16)(byte_count / 2);
    if (out_reg_count) *out_reg_count = total_regs;

    if (out_regs && max_regs > 0) {
        u16 copy_regs = (total_regs < max_regs) ? total_regs : max_regs;
        for (u16 i = 0; i < copy_regs; ++i) {
            out_regs[i] = ((u16)frame[3 + i * 2] << 8) | (u16)frame[4 + i * 2];
        }
    }
    return true;
}

usize cp_modbus_tcp_build_read_holding_registers(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count) {
    if (!out || cap < 12) return 0;
    /* MBAP Header (7 bytes) */
    out[0] = (u8)(tx_id >> 8);
    out[1] = (u8)(tx_id & 0xFF);
    out[2] = 0x00; /* Protocol ID (0 = Modbus) */
    out[3] = 0x00;
    out[4] = 0x00; /* Length: 6 bytes to follow */
    out[5] = 0x06;
    out[6] = unit_id;
    /* PDU */
    out[7] = 0x03; /* Function code: Read Holding Registers */
    out[8] = (u8)(start_addr >> 8);
    out[9] = (u8)(start_addr & 0xFF);
    out[10] = (u8)(count >> 8);
    out[11] = (u8)(count & 0xFF);
    return 12;
}

usize cp_modbus_tcp_build_write_multiple_registers(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count, const u16 *values) {
    usize required = 13 + (usize)count * 2;
    if (!out || cap < required || !values) return 0;

    u16 remaining_len = (u16)(7 + (usize)count * 2);

    /* MBAP Header */
    out[0] = (u8)(tx_id >> 8);
    out[1] = (u8)(tx_id & 0xFF);
    out[2] = 0x00;
    out[3] = 0x00;
    out[4] = (u8)(remaining_len >> 8);
    out[5] = (u8)(remaining_len & 0xFF);
    out[6] = unit_id;

    /* PDU */
    out[7] = 0x10; /* Function code: Write Multiple Registers */
    out[8] = (u8)(start_addr >> 8);
    out[9] = (u8)(start_addr & 0xFF);
    out[10] = (u8)(count >> 8);
    out[11] = (u8)(count & 0xFF);
    out[12] = (u8)(count * 2);

    for (usize i = 0; i < count; ++i) {
        out[13 + i * 2] = (u8)(values[i] >> 8);
        out[14 + i * 2] = (u8)(values[i] & 0xFF);
    }
    return required;
}

bool cp_modbus_tcp_parse_read_response(const u8 *frame, usize len, u16 expected_tx_id, u8 expected_unit_id, u16 *out_regs, u16 max_regs, u16 *out_reg_count) {
    if (!frame || len < 9) return false;
    u16 tx_id = ((u16)frame[0] << 8) | (u16)frame[1];
    u16 proto = ((u16)frame[2] << 8) | (u16)frame[3];
    u16 length = ((u16)frame[4] << 8) | (u16)frame[5];
    u8 unit_id = frame[6];

    if (expected_tx_id != 0 && tx_id != expected_tx_id) return false;
    if (proto != 0) return false;
    if (expected_unit_id != 0 && unit_id != expected_unit_id) return false;
    if (len < (usize)(6 + length)) return false;

    u8 fc = frame[7];
    if (fc != 0x03) return false;

    u8 byte_count = frame[8];
    u16 total_regs = (u16)(byte_count / 2);
    if (out_reg_count) *out_reg_count = total_regs;

    if (out_regs && max_regs > 0) {
        u16 copy_regs = (total_regs < max_regs) ? total_regs : max_regs;
        for (u16 i = 0; i < copy_regs; ++i) {
            out_regs[i] = ((u16)frame[9 + i * 2] << 8) | (u16)frame[10 + i * 2];
        }
    }
    return true;
}

/* -------------------------------------------------------------------------
 * Industrial Protocols: CAN Bus
 * ------------------------------------------------------------------------- */
void cp_can_pack(cp_can_frame_t *frame, u32 id, const void *data, u8 len, bool is_extended) {
    if (!frame) return;
    frame->can_id = id;
    frame->can_dlc = (len > 8) ? 8 : len;
    frame->is_extended = is_extended;
    frame->is_rtr = false;
    memset(frame->data, 0, 8);
    if (data && frame->can_dlc > 0) {
        memcpy(frame->data, data, frame->can_dlc);
    }
}

bool cp_can_unpack(const cp_can_frame_t *frame, u32 *out_id, void *out_data, u8 *out_len, bool *out_is_extended) {
    if (!frame) return false;
    if (out_id) *out_id = frame->can_id;
    if (out_len) *out_len = frame->can_dlc;
    if (out_is_extended) *out_is_extended = frame->is_extended;
    if (out_data && frame->can_dlc > 0) {
        memcpy(out_data, frame->data, frame->can_dlc);
    }
    return true;
}

cp_socket_t cp_can_open(const char *iface_name) {
    if (!iface_name) return CP_INVALID_SOCKET;
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) return CP_INVALID_SOCKET;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface_name, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }
    return fd;
}

bool cp_can_send(cp_socket_t fd, const cp_can_frame_t *frame) {
    if (fd < 0 || !frame) return false;
    struct can_frame raw;
    memset(&raw, 0, sizeof(raw));
    raw.can_id = frame->can_id;
    if (frame->is_extended) raw.can_id |= CAN_EFF_FLAG;
    if (frame->is_rtr) raw.can_id |= CAN_RTR_FLAG;
    raw.can_dlc = frame->can_dlc;
    memcpy(raw.data, frame->data, frame->can_dlc);

    return write(fd, &raw, sizeof(raw)) == (ssize_t)sizeof(raw);
}

bool cp_can_recv(cp_socket_t fd, cp_can_frame_t *out_frame, u32 timeout_ms) {
    if (fd < 0 || !out_frame) return false;
    if (timeout_ms > 0) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
        int r = poll(&pfd, 1, (int)timeout_ms);
        if (r <= 0) return false;
    }
    struct can_frame raw;
    ssize_t n = read(fd, &raw, sizeof(raw));
    if (n != (ssize_t)sizeof(raw)) return false;

    out_frame->is_extended = (raw.can_id & CAN_EFF_FLAG) != 0;
    out_frame->is_rtr = (raw.can_id & CAN_RTR_FLAG) != 0;
    out_frame->can_id = raw.can_id & (out_frame->is_extended ? CAN_EFF_MASK : CAN_SFF_MASK);
    out_frame->can_dlc = raw.can_dlc;
    memcpy(out_frame->data, raw.data, raw.can_dlc);
    return true;
}

bool cp_can_set_filter(cp_socket_t fd, u32 can_id, u32 can_mask) {
    if (fd < 0) return false;
    struct can_filter rfilter;
    rfilter.can_id = can_id;
    rfilter.can_mask = can_mask;
    return setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) == 0;
}

/* --- Source: cp_tui.c --- */
/**
 * @file cp_tui.c
 * @brief High-performance, zero-dependency double-buffered Terminal UI (TUI) engine.
 */


#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <poll.h>

/* -------------------------------------------------------------------------
 * Global Crash-Safety State
 * ------------------------------------------------------------------------- */
static cp_tui_t *g_active_tui = NULL;

static void cp_tui_signal_handler(int sig) {
    if (g_active_tui) {
        cp_tui_shutdown(g_active_tui);
    }
    _exit(128 + sig);
}

/* -------------------------------------------------------------------------
 * Internal Output Buffer Helpers
 * ------------------------------------------------------------------------- */
static void cp_tui_out_write(cp_tui_t *tui, const char *str, usize len) {
    if (tui->out_len + len >= tui->out_cap) {
        usize new_cap = (tui->out_cap + len) * 2;
        char *nb = (char *)realloc(tui->out_buffer, new_cap);
        if (!nb) return;
        tui->out_buffer = nb;
        tui->out_cap = new_cap;
    }
    memcpy(tui->out_buffer + tui->out_len, str, len);
    tui->out_len += len;
}

static void cp_tui_out_printf(cp_tui_t *tui, const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > 0) {
        cp_tui_out_write(tui, buf, (usize)n);
    }
}

static void cp_tui_emit_sgr(cp_tui_t *tui, u8 fg, u8 bg, u8 attrs) {
    cp_tui_out_write(tui, "\033[0", 3);

    if (attrs & TUI_ATTR_BOLD)      cp_tui_out_write(tui, ";1", 2);
    if (attrs & TUI_ATTR_DIM)       cp_tui_out_write(tui, ";2", 2);
    if (attrs & TUI_ATTR_UNDERLINE) cp_tui_out_write(tui, ";4", 2);
    if (attrs & TUI_ATTR_REVERSE)   cp_tui_out_write(tui, ";7", 2);

    /* Foreground */
    if (fg >= 1 && fg <= 8) {
        cp_tui_out_printf(tui, ";%d", 29 + fg);
    } else if (fg >= 9 && fg <= 16) {
        cp_tui_out_printf(tui, ";%d", 81 + fg);
    }

    /* Background */
    if (bg >= 1 && bg <= 8) {
        cp_tui_out_printf(tui, ";%d", 39 + bg);
    } else if (bg >= 9 && bg <= 16) {
        cp_tui_out_printf(tui, ";%d", 91 + bg);
    }

    cp_tui_out_write(tui, "m", 1);
}

/* -------------------------------------------------------------------------
 * Lifecycle Management
 * ------------------------------------------------------------------------- */
cp_tui_t *cp_tui_create(usize width, usize height, bool is_headless) {
    if ((width == 0 || height == 0) && !is_headless) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
            width = (usize)ws.ws_col;
            height = (usize)ws.ws_row;
        }
    }
    if (width == 0)  width = 80;
    if (height == 0) height = 24;

    cp_tui_t *tui = (cp_tui_t *)calloc(1, sizeof(cp_tui_t));
    if (!tui) return NULL;

    tui->width = width;
    tui->height = height;
    tui->is_headless = is_headless;

    usize cell_count = width * height;
    tui->front_buffer = (cp_tui_cell_t *)calloc(cell_count, sizeof(cp_tui_cell_t));
    tui->back_buffer = (cp_tui_cell_t *)calloc(cell_count, sizeof(cp_tui_cell_t));

    tui->out_cap = cell_count * 16;
    if (tui->out_cap < 8192) tui->out_cap = 8192;
    tui->out_buffer = (char *)malloc(tui->out_cap);
    tui->out_len = 0;

    if (!tui->front_buffer || !tui->back_buffer || !tui->out_buffer) {
        cp_tui_shutdown(tui);
        return NULL;
    }

    /* Initialize front buffer with empty glyphs so initial present renders cleanly */
    for (usize i = 0; i < cell_count; ++i) {
        tui->front_buffer[i].glyph[0] = '\0';
        tui->front_buffer[i].fg = 255;
        tui->front_buffer[i].bg = 255;
        tui->front_buffer[i].attrs = 255;

        tui->back_buffer[i].glyph[0] = ' ';
        tui->back_buffer[i].glyph[1] = '\0';
    }

    if (!is_headless && isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
        if (tcgetattr(STDIN_FILENO, &tui->orig_termios) == 0) {
            struct termios raw = tui->orig_termios;
            raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
            raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
            raw.c_cflag |= (CS8);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            tui->raw_mode_active = true;

            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = cp_tui_signal_handler;
            sigaction(SIGINT, &sa, NULL);
            sigaction(SIGTERM, &sa, NULL);

            /* Switch to alternate screen and hide cursor */
            ssize_t w = write(STDOUT_FILENO, "\033[?1049h\033[?25l", 14);
            CP_UNUSED(w);

            g_active_tui = tui;
        }
    }

    return tui;
}

cp_tui_t *cp_tui_init(usize width, usize height) {
    return cp_tui_create(width, height, false);
}

void cp_tui_shutdown(cp_tui_t *tui) {
    if (!tui) return;

    if (tui->raw_mode_active) {
        /* Show cursor and exit alternate screen */
        ssize_t w = write(STDOUT_FILENO, "\033[?25h\033[?1049l", 14);
        CP_UNUSED(w);
        tcsetattr(STDIN_FILENO, TCSANOW, &tui->orig_termios);
        tui->raw_mode_active = false;
    }

    if (g_active_tui == tui) {
        g_active_tui = NULL;
    }

    free(tui->front_buffer);
    free(tui->back_buffer);
    free(tui->out_buffer);
    free(tui);
}

/* -------------------------------------------------------------------------
 * Rendering & Differential Flusher
 * ------------------------------------------------------------------------- */
void cp_tui_clear(cp_tui_t *tui, u8 bg) {
    if (!tui) return;
    usize count = tui->width * tui->height;
    for (usize i = 0; i < count; ++i) {
        tui->back_buffer[i].glyph[0] = ' ';
        tui->back_buffer[i].glyph[1] = '\0';
        tui->back_buffer[i].fg = TUI_COLOR_DEFAULT;
        tui->back_buffer[i].bg = bg;
        tui->back_buffer[i].attrs = TUI_ATTR_NONE;
    }
}

usize cp_tui_present(cp_tui_t *tui) {
    if (!tui) return 0;
    tui->out_len = 0;

    u8 cur_fg = 255;
    u8 cur_bg = 255;
    u8 cur_attrs = 255;
    usize cur_x = (usize)-1;
    usize cur_y = (usize)-1;

    for (usize y = 0; y < tui->height; ++y) {
        for (usize x = 0; x < tui->width; ++x) {
            usize idx = y * tui->width + x;
            cp_tui_cell_t *b = &tui->back_buffer[idx];
            cp_tui_cell_t *f = &tui->front_buffer[idx];

            if (memcmp(b, f, sizeof(cp_tui_cell_t)) != 0) {
                /* Cursor position changed */
                if (cur_y != y || cur_x != x) {
                    cp_tui_out_printf(tui, "\033[%zu;%zuH", y + 1, x + 1);
                    cur_x = x;
                    cur_y = y;
                }

                /* SGR attribute changed */
                if (b->fg != cur_fg || b->bg != cur_bg || b->attrs != cur_attrs) {
                    cp_tui_emit_sgr(tui, b->fg, b->bg, b->attrs);
                    cur_fg = b->fg;
                    cur_bg = b->bg;
                    cur_attrs = b->attrs;
                }

                /* Write glyph */
                const char *g = (b->glyph[0] != '\0') ? b->glyph : " ";
                usize glen = strlen(g);
                cp_tui_out_write(tui, g, glen);

                cur_x++;
                *f = *b;
            }
        }
    }

    if (cur_fg != 255 || cur_bg != 255 || cur_attrs != 255) {
        cp_tui_out_write(tui, "\033[0m", 4);
    }

    if (!tui->is_headless && tui->out_len > 0) {
        ssize_t w = write(STDOUT_FILENO, tui->out_buffer, tui->out_len);
        CP_UNUSED(w);
    }

    return tui->out_len;
}

/* -------------------------------------------------------------------------
 * Drawing Primitives
 * ------------------------------------------------------------------------- */
void cp_tui_set_cell(cp_tui_t *tui, usize x, usize y, const char *glyph, u8 fg, u8 bg, u8 attrs) {
    if (!tui || x >= tui->width || y >= tui->height) return;
    usize idx = y * tui->width + x;
    cp_tui_cell_t *cell = &tui->back_buffer[idx];

    cell->fg = fg;
    cell->bg = bg;
    cell->attrs = attrs;
    memset(cell->glyph, 0, sizeof(cell->glyph));

    if (glyph && glyph[0] != '\0') {
        usize glen = strlen(glyph);
        if (glen >= sizeof(cell->glyph)) glen = sizeof(cell->glyph) - 1;
        memcpy(cell->glyph, glyph, glen);
        cell->glyph[glen] = '\0';
    } else {
        cell->glyph[0] = ' ';
        cell->glyph[1] = '\0';
    }
}

void cp_tui_print(cp_tui_t *tui, usize x, usize y, const char *str, u8 fg, u8 bg, u8 attrs) {
    if (!tui || !str || y >= tui->height) return;

    usize cur_x = x;
    const char *p = str;

    while (*p && cur_x < tui->width) {
        char glyph[8];
        memset(glyph, 0, sizeof(glyph));

        unsigned char c = (unsigned char)*p;
        usize char_len = 1;
        if ((c & 0x80) == 0) {
            char_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        }

        for (usize i = 0; i < char_len && *p; ++i) {
            glyph[i] = *p++;
        }

        cp_tui_set_cell(tui, cur_x, y, glyph, fg, bg, attrs);
        cur_x++;
    }
}

void cp_tui_box(cp_tui_t *tui, usize x, usize y, usize w, usize h, const char *title, u8 fg, u8 bg) {
    if (!tui || w < 2 || h < 2 || x >= tui->width || y >= tui->height) return;

    usize right = (x + w <= tui->width) ? (x + w - 1) : (tui->width - 1);
    usize bottom = (y + h <= tui->height) ? (y + h - 1) : (tui->height - 1);

    /* Unicode Box Characters */
    const char *TOP_LEFT     = "\xe2\x94\x8c"; /* ┌ */
    const char *TOP_RIGHT    = "\xe2\x94\x90"; /* ┐ */
    const char *BOTTOM_LEFT  = "\xe2\x94\x94"; /* └ */
    const char *BOTTOM_RIGHT = "\xe2\x94\x98"; /* ┘ */
    const char *HORIZONTAL   = "\xe2\x94\x80"; /* ─ */
    const char *VERTICAL     = "\xe2\x94\x82"; /* │ */

    /* Corners */
    cp_tui_set_cell(tui, x, y, TOP_LEFT, fg, bg, TUI_ATTR_NONE);
    cp_tui_set_cell(tui, right, y, TOP_RIGHT, fg, bg, TUI_ATTR_NONE);
    cp_tui_set_cell(tui, x, bottom, BOTTOM_LEFT, fg, bg, TUI_ATTR_NONE);
    cp_tui_set_cell(tui, right, bottom, BOTTOM_RIGHT, fg, bg, TUI_ATTR_NONE);

    /* Horizontal lines */
    for (usize col = x + 1; col < right; ++col) {
        cp_tui_set_cell(tui, col, y, HORIZONTAL, fg, bg, TUI_ATTR_NONE);
        cp_tui_set_cell(tui, col, bottom, HORIZONTAL, fg, bg, TUI_ATTR_NONE);
    }

    /* Vertical lines & interior clear */
    for (usize row = y + 1; row < bottom; ++row) {
        cp_tui_set_cell(tui, x, row, VERTICAL, fg, bg, TUI_ATTR_NONE);
        cp_tui_set_cell(tui, right, row, VERTICAL, fg, bg, TUI_ATTR_NONE);
        for (usize col = x + 1; col < right; ++col) {
            cp_tui_set_cell(tui, col, row, " ", fg, bg, TUI_ATTR_NONE);
        }
    }

    /* Centered Title */
    if (title && title[0] != '\0') {
        usize title_len = strlen(title);
        usize inner_w = right - x - 1;
        if (title_len + 4 <= inner_w) {
            usize start_col = x + 2 + (inner_w - title_len - 4) / 2;
            cp_tui_set_cell(tui, start_col, y, " ", fg, bg, TUI_ATTR_NONE);
            cp_tui_print(tui, start_col + 1, y, title, fg, bg, TUI_ATTR_BOLD);
            cp_tui_set_cell(tui, start_col + 1 + title_len, y, " ", fg, bg, TUI_ATTR_NONE);
        }
    }
}

void cp_tui_progress_bar(cp_tui_t *tui, usize x, usize y, usize width, f32 percentage, const char *label, u8 fg, u8 bg) {
    if (!tui || width < 10 || x >= tui->width || y >= tui->height) return;

    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 1.0f) percentage = 1.0f;

    char pct_str[16];
    int pct_val = (int)(percentage * 100.0f + 0.5f);
    snprintf(pct_str, sizeof(pct_str), " %3d%%", pct_val);
    usize pct_len = strlen(pct_str);

    usize label_len = label ? strlen(label) : 0;
    usize prefix_w = (label_len > 0) ? (label_len + 1) : 0;

    if (prefix_w > 0) {
        cp_tui_print(tui, x, y, label, fg, bg, TUI_ATTR_BOLD);
        cp_tui_set_cell(tui, x + label_len, y, " ", fg, bg, TUI_ATTR_NONE);
    }

    usize bar_start = x + prefix_w;
    usize bar_w = (width > (prefix_w + pct_len + 2)) ? (width - prefix_w - pct_len - 2) : 4;

    cp_tui_set_cell(tui, bar_start, y, "[", fg, bg, TUI_ATTR_NONE);

    usize filled_units = (usize)((f32)bar_w * percentage);
    const char *FILLED = "\xe2\x96\x88"; /* █ */
    const char *EMPTY  = "\xe2\x96\x91"; /* ░ */

    for (usize i = 0; i < bar_w; ++i) {
        if (i < filled_units) {
            cp_tui_set_cell(tui, bar_start + 1 + i, y, FILLED, TUI_COLOR_BRIGHT_GREEN, bg, TUI_ATTR_BOLD);
        } else {
            cp_tui_set_cell(tui, bar_start + 1 + i, y, EMPTY, TUI_COLOR_BRIGHT_BLACK, bg, TUI_ATTR_DIM);
        }
    }

    cp_tui_set_cell(tui, bar_start + 1 + bar_w, y, "]", fg, bg, TUI_ATTR_NONE);
    cp_tui_print(tui, bar_start + 2 + bar_w, y, pct_str, fg, bg, TUI_ATTR_BOLD);
}

/* -------------------------------------------------------------------------
 * Input & Interactive Components
 * ------------------------------------------------------------------------- */
cp_tui_key_t cp_tui_read_key(cp_tui_t *tui, u32 timeout_ms) {
    if (!tui || tui->is_headless) return TUI_KEY_NONE;

    struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN, .revents = 0 };
    int r = poll(&pfd, 1, (int)timeout_ms);
    if (r <= 0) return TUI_KEY_NONE;

    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return TUI_KEY_NONE;

    if (c == 27) { /* Escape sequence */
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return TUI_KEY_ESCAPE;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return TUI_KEY_ESCAPE;

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return TUI_KEY_UP;
                case 'B': return TUI_KEY_DOWN;
                case 'C': return TUI_KEY_RIGHT;
                case 'D': return TUI_KEY_LEFT;
                default: break;
            }
        }
        return TUI_KEY_ESCAPE;
    }

    if (c == '\n' || c == '\r') return TUI_KEY_ENTER;
    if (c == 127 || c == 8)     return TUI_KEY_BACKSPACE;
    if (c == '\t')              return TUI_KEY_TAB;

    return (cp_tui_key_t)(unsigned char)c;
}

bool cp_tui_menu(cp_tui_t *tui, usize x, usize y, usize w, const char *title, const char **options, usize count, usize *selected_index) {
    if (!tui || !options || count == 0 || !selected_index) return false;

    usize h = count + 2;
    usize curr = (*selected_index < count) ? *selected_index : 0;

    while (1) {
        cp_tui_box(tui, x, y, w, h, title, TUI_COLOR_BRIGHT_BLUE, TUI_COLOR_DEFAULT);

        for (usize i = 0; i < count; ++i) {
            char line[256];
            bool is_selected = (i == curr);
            snprintf(line, sizeof(line), " %c %-.*s", is_selected ? '>' : ' ', (int)(w - 6), options[i]);
            u8 fg = is_selected ? TUI_COLOR_BRIGHT_WHITE : TUI_COLOR_DEFAULT;
            u8 bg = is_selected ? TUI_COLOR_BLUE : TUI_COLOR_DEFAULT;
            u8 attr = is_selected ? (TUI_ATTR_BOLD | TUI_ATTR_REVERSE) : TUI_ATTR_NONE;
            cp_tui_print(tui, x + 1, y + 1 + i, line, fg, bg, attr);
        }

        cp_tui_present(tui);

        if (tui->is_headless) {
            *selected_index = curr;
            return true;
        }

        cp_tui_key_t key = cp_tui_read_key(tui, 100);
        if (key == TUI_KEY_UP || key == (cp_tui_key_t)'k') {
            curr = (curr == 0) ? (count - 1) : (curr - 1);
        } else if (key == TUI_KEY_DOWN || key == (cp_tui_key_t)'j') {
            curr = (curr + 1) % count;
        } else if (key == TUI_KEY_ENTER) {
            *selected_index = curr;
            return true;
        } else if (key == TUI_KEY_ESCAPE || key == (cp_tui_key_t)'q') {
            return false;
        }
    }
}

bool cp_tui_prompt(cp_tui_t *tui, usize x, usize y, usize w, const char *prompt, char *out_buf, usize cap) {
    if (!tui || !out_buf || cap == 0) return false;

    usize len = strlen(out_buf);
    usize h = 3;

    while (1) {
        cp_tui_box(tui, x, y, w, h, prompt, TUI_COLOR_BRIGHT_CYAN, TUI_COLOR_DEFAULT);

        /* Draw current text with cursor */
        char display[256];
        snprintf(display, sizeof(display), " %-.*s_", (int)(w - 5), out_buf);
        cp_tui_print(tui, x + 1, y + 1, display, TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_BOLD);

        cp_tui_present(tui);

        if (tui->is_headless) return true;

        cp_tui_key_t key = cp_tui_read_key(tui, 100);
        if (key == TUI_KEY_ENTER) {
            return true;
        } else if (key == TUI_KEY_ESCAPE) {
            return false;
        } else if (key == TUI_KEY_BACKSPACE) {
            if (len > 0) {
                out_buf[--len] = '\0';
            }
        } else if (key >= 32 && key <= 126 && len + 1 < cap) {
            out_buf[len++] = (char)key;
            out_buf[len] = '\0';
        }
    }
}

/* --- Source: cp_hash.c --- */
/**
 * @file cp_hash.c
 * @brief Implementation of FNV-1a, fast Hash64, and standard SHA-256.
 */

#include <string.h>

/* -------------------------------------------------------------------------
 * Non-Cryptographic Fast Hashes
 * ------------------------------------------------------------------------- */
u32 cp_hash_fnv1a32(const void *data, usize len) {
    if (!data) return 0;
    const u8 *p = (const u8 *)data;
    u32 hash = 0x811c9dc5u;
    for (usize i = 0; i < len; ++i) {
        hash ^= p[i];
        hash *= 0x01000193u;
    }
    return hash;
}

u64 cp_hash_fnv1a64(const void *data, usize len) {
    if (!data) return 0;
    const u8 *p = (const u8 *)data;
    u64 hash = 0xcbf29ce484222325ULL;
    for (usize i = 0; i < len; ++i) {
        hash ^= p[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

u64 cp_hash64(const void *data, usize len) {
    if (!data) return 0;
    const u8 *p = (const u8 *)data;
    u64 h = 0x243f6a8885a308d3ULL ^ (len * 0x9e3779b97f4a7c15ULL);

    while (len >= 8) {
        u64 k;
        memcpy(&k, p, 8);
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53ULL;
        k ^= k >> 33;
        h ^= k;
        h *= 0x9ddfea08eb382d69ULL;
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        u64 k = 0;
        memcpy(&k, p, len);
        h ^= k;
        h *= 0x9ddfea08eb382d69ULL;
    }

    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    return h;
}

/* -------------------------------------------------------------------------
 * SHA-256 Implementation (FIPS 180-4)
 * ------------------------------------------------------------------------- */
static const u32 K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define GAMMA0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define GAMMA1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static void cp_sha256_transform(cp_sha256_ctx_t *ctx, const u8 block[64]) {
    u32 w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = ((u32)block[i * 4] << 24) |
               ((u32)block[i * 4 + 1] << 16) |
               ((u32)block[i * 4 + 2] << 8) |
               ((u32)block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        w[i] = GAMMA1(w[i - 2]) + w[i - 7] + GAMMA0(w[i - 15]) + w[i - 16];
    }

    u32 a = ctx->state[0];
    u32 b = ctx->state[1];
    u32 c = ctx->state[2];
    u32 d = ctx->state[3];
    u32 e = ctx->state[4];
    u32 f = ctx->state[5];
    u32 g = ctx->state[6];
    u32 h = ctx->state[7];

    for (int i = 0; i < 64; ++i) {
        u32 t1 = h + SIGMA1(e) + CH(e, f, g) + K[i] + w[i];
        u32 t2 = SIGMA0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void cp_sha256_init(cp_sha256_ctx_t *ctx) {
    if (!ctx) return;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

void cp_sha256_update(cp_sha256_ctx_t *ctx, const void *data, usize len) {
    if (!ctx || !data || len == 0) return;
    const u8 *p = (const u8 *)data;
    usize buf_idx = (usize)(ctx->count & 0x3F);
    ctx->count += len;

    if (buf_idx > 0) {
        usize space = 64 - buf_idx;
        if (len < space) {
            memcpy(ctx->buffer + buf_idx, p, len);
            return;
        }
        memcpy(ctx->buffer + buf_idx, p, space);
        cp_sha256_transform(ctx, ctx->buffer);
        p += space;
        len -= space;
    }

    while (len >= 64) {
        cp_sha256_transform(ctx, p);
        p += 64;
        len -= 64;
    }

    if (len > 0) {
        memcpy(ctx->buffer, p, len);
    }
}

void cp_sha256_final(cp_sha256_ctx_t *ctx, u8 out_hash[32]) {
    if (!ctx || !out_hash) return;
    u64 total_bits = ctx->count * 8;
    usize buf_idx = (usize)(ctx->count & 0x3F);

    ctx->buffer[buf_idx++] = 0x80;
    if (buf_idx > 56) {
        memset(ctx->buffer + buf_idx, 0, 64 - buf_idx);
        cp_sha256_transform(ctx, ctx->buffer);
        buf_idx = 0;
    }
    memset(ctx->buffer + buf_idx, 0, 56 - buf_idx);

    for (int i = 7; i >= 0; --i) {
        ctx->buffer[56 + (7 - i)] = (u8)((total_bits >> (i * 8)) & 0xFF);
    }
    cp_sha256_transform(ctx, ctx->buffer);

    for (int i = 0; i < 8; ++i) {
        out_hash[i * 4]     = (u8)((ctx->state[i] >> 24) & 0xFF);
        out_hash[i * 4 + 1] = (u8)((ctx->state[i] >> 16) & 0xFF);
        out_hash[i * 4 + 2] = (u8)((ctx->state[i] >> 8) & 0xFF);
        out_hash[i * 4 + 3] = (u8)(ctx->state[i] & 0xFF);
    }
}

void cp_sha256(const void *data, usize len, u8 out_hash[32]) {
    cp_sha256_ctx_t ctx;
    cp_sha256_init(&ctx);
    cp_sha256_update(&ctx, data, len);
    cp_sha256_final(&ctx, out_hash);
}

void cp_sha256_hex(const u8 in_hash[32], char out_hex[65]) {
    if (!in_hash || !out_hex) return;
    static const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < 32; ++i) {
        out_hex[i * 2]     = hex_chars[(in_hash[i] >> 4) & 0x0F];
        out_hex[i * 2 + 1] = hex_chars[in_hash[i] & 0x0F];
    }
    out_hex[64] = '\0';
}

/* --- Source: cp_map.c --- */
/**
 * @file cp_map.c
 * @brief Implementation of open-addressing Hash Map with linear probing.
 */

#include <string.h>
#include <stdlib.h>

static usize next_power_of_two(usize x) {
    if (x <= 16) return 16;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

static char *cp_map_dup_key(const char *key, cp_arena_t *arena) {
    if (!key) return NULL;
    usize len = strlen(key);
    char *copy;
    if (arena) {
        copy = (char *)ArenaAlloc(arena, len + 1);
    } else {
        copy = (char *)malloc(len + 1);
    }
    if (copy) {
        memcpy(copy, key, len + 1);
    }
    return copy;
}

cp_map_t *cp_map_create(usize initial_capacity, cp_arena_t *arena) {
    usize cap = next_power_of_two(initial_capacity);
    cp_map_t *map;
    if (arena) {
        map = (cp_map_t *)ArenaAlloc(arena, sizeof(cp_map_t));
        map->entries = (cp_map_entry_t *)ArenaAlloc(arena, cap * sizeof(cp_map_entry_t));
    } else {
        map = (cp_map_t *)calloc(1, sizeof(cp_map_t));
        if (!map) return NULL;
        map->entries = (cp_map_entry_t *)calloc(cap, sizeof(cp_map_entry_t));
        if (!map->entries) {
            free(map);
            return NULL;
        }
    }
    memset(map->entries, 0, cap * sizeof(cp_map_entry_t));
    map->capacity = cap;
    map->count = 0;
    map->tombstones = 0;
    map->arena = arena;
    return map;
}

void cp_map_destroy(cp_map_t *map) {
    if (!map) return;
    if (!map->arena) {
        for (usize i = 0; i < map->capacity; ++i) {
            if (map->entries[i].occupied && !map->entries[i].deleted) {
                free(map->entries[i].key);
            }
        }
        free(map->entries);
        free(map);
    }
}

static bool cp_map_resize(cp_map_t *map, usize new_cap) {
    cp_map_entry_t *old_entries = map->entries;
    usize old_cap = map->capacity;

    cp_map_entry_t *new_entries;
    if (map->arena) {
        new_entries = (cp_map_entry_t *)ArenaAlloc(map->arena, new_cap * sizeof(cp_map_entry_t));
    } else {
        new_entries = (cp_map_entry_t *)calloc(new_cap, sizeof(cp_map_entry_t));
    }
    if (!new_entries) return false;
    memset(new_entries, 0, new_cap * sizeof(cp_map_entry_t));

    usize mask = new_cap - 1;
    for (usize i = 0; i < old_cap; ++i) {
        if (old_entries[i].occupied && !old_entries[i].deleted) {
            usize idx = old_entries[i].hash & mask;
            while (new_entries[idx].occupied) {
                idx = (idx + 1) & mask;
            }
            new_entries[idx] = old_entries[i];
        }
    }

    if (!map->arena) {
        free(old_entries);
    }

    map->entries = new_entries;
    map->capacity = new_cap;
    map->tombstones = 0;
    return true;
}

bool cp_map_set(cp_map_t *map, const char *key, void *value) {
    if (!map || !key) return false;

    /* Resize when load factor > 70% */
    if ((map->count + map->tombstones + 1) * 10 >= map->capacity * 7) {
        usize new_cap = map->capacity * 2;
        if (!cp_map_resize(map, new_cap)) return false;
    }

    u64 hash = Hash64(key, strlen(key));
    usize mask = map->capacity - 1;
    usize idx = hash & mask;
    usize tombstone_idx = (usize)-1;

    while (map->entries[idx].occupied) {
        if (map->entries[idx].deleted) {
            if (tombstone_idx == (usize)-1) {
                tombstone_idx = idx;
            }
        } else if (map->entries[idx].hash == hash && strcmp(map->entries[idx].key, key) == 0) {
            /* Update existing key value */
            map->entries[idx].value = value;
            return true;
        }
        idx = (idx + 1) & mask;
    }

    /* Reuse tombstone if found, else use first empty slot */
    usize insert_idx = (tombstone_idx != (usize)-1) ? tombstone_idx : idx;
    char *k = cp_map_dup_key(key, map->arena);
    if (!k) return false;

    if (map->entries[insert_idx].deleted) {
        map->tombstones--;
    }
    map->entries[insert_idx].key = k;
    map->entries[insert_idx].value = value;
    map->entries[insert_idx].hash = hash;
    map->entries[insert_idx].occupied = true;
    map->entries[insert_idx].deleted = false;
    map->count++;
    return true;
}

void *cp_map_get(const cp_map_t *map, const char *key) {
    if (!map || !key || map->count == 0) return NULL;

    u64 hash = Hash64(key, strlen(key));
    usize mask = map->capacity - 1;
    usize idx = hash & mask;

    while (map->entries[idx].occupied) {
        if (!map->entries[idx].deleted && map->entries[idx].hash == hash && strcmp(map->entries[idx].key, key) == 0) {
            return map->entries[idx].value;
        }
        idx = (idx + 1) & mask;
    }
    return NULL;
}

bool cp_map_has(const cp_map_t *map, const char *key) {
    return cp_map_get(map, key) != NULL;
}

bool cp_map_delete(cp_map_t *map, const char *key) {
    if (!map || !key || map->count == 0) return false;

    u64 hash = Hash64(key, strlen(key));
    usize mask = map->capacity - 1;
    usize idx = hash & mask;

    while (map->entries[idx].occupied) {
        if (!map->entries[idx].deleted && map->entries[idx].hash == hash && strcmp(map->entries[idx].key, key) == 0) {
            if (!map->arena) {
                free(map->entries[idx].key);
            }
            map->entries[idx].key = NULL;
            map->entries[idx].value = NULL;
            map->entries[idx].deleted = true;
            map->count--;
            map->tombstones++;
            return true;
        }
        idx = (idx + 1) & mask;
    }
    return false;
}

void cp_map_clear(cp_map_t *map) {
    if (!map) return;
    for (usize i = 0; i < map->capacity; ++i) {
        if (map->entries[i].occupied && !map->entries[i].deleted) {
            if (!map->arena) {
                free(map->entries[i].key);
            }
        }
    }
    memset(map->entries, 0, map->capacity * sizeof(cp_map_entry_t));
    map->count = 0;
    map->tombstones = 0;
}

usize cp_map_len(const cp_map_t *map) {
    return map ? map->count : 0;
}

/* --- Source: cp_args.c --- */
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

/* --- Source: cp_time.c --- */
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

/* --- Source: cp_json.c --- */
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

/* --- Source: cp_stack.c --- */
/**
 * @file cp_stack.c
 * @brief High-performance L1 cache-friendly Stack implementation.
 */

#include <stdlib.h>
#include <string.h>

#define CP_STACK_DEFAULT_CAPACITY 16

cp_stack_t *cp_stack_create(usize initial_capacity, cp_arena_t *arena) {
    if (initial_capacity == 0) {
        initial_capacity = CP_STACK_DEFAULT_CAPACITY;
    }

    cp_stack_t *stack = NULL;
    if (arena) {
        stack = (cp_stack_t *)ArenaAlloc(arena, sizeof(cp_stack_t));
        if (!stack) return NULL;
        stack->items = (void **)ArenaAlloc(arena, initial_capacity * sizeof(void *));
        if (!stack->items) return NULL;
    } else {
        stack = (cp_stack_t *)malloc(sizeof(cp_stack_t));
        if (!stack) return NULL;
        stack->items = (void **)malloc(initial_capacity * sizeof(void *));
        if (!stack->items) {
            free(stack);
            return NULL;
        }
    }

    stack->capacity = initial_capacity;
    stack->top = 0;
    stack->arena = arena;
    return stack;
}

void cp_stack_destroy(cp_stack_t *stack) {
    if (!stack) return;
    if (!stack->arena) {
        if (stack->items) {
            free(stack->items);
        }
        free(stack);
    }
}

bool cp_stack_push(cp_stack_t *stack, void *item) {
    if (!stack) return false;

    if (stack->top >= stack->capacity) {
        usize new_cap = stack->capacity * 2;
        if (new_cap < CP_STACK_DEFAULT_CAPACITY) new_cap = CP_STACK_DEFAULT_CAPACITY;

        if (stack->arena) {
            void **new_items = (void **)ArenaAlloc(stack->arena, new_cap * sizeof(void *));
            if (!new_items) return false;
            memcpy(new_items, stack->items, stack->top * sizeof(void *));
            stack->items = new_items;
        } else {
            void **new_items = (void **)realloc(stack->items, new_cap * sizeof(void *));
            if (!new_items) return false;
            stack->items = new_items;
        }
        stack->capacity = new_cap;
    }

    stack->items[stack->top++] = item;
    return true;
}

void *cp_stack_pop(cp_stack_t *stack) {
    if (!stack || stack->top == 0) return NULL;
    stack->top--;
    return stack->items[stack->top];
}

void *cp_stack_get(const cp_stack_t *stack, usize index_from_top) {
    if (!stack || index_from_top >= stack->top) return NULL;
    usize actual_idx = stack->top - 1 - index_from_top;
    return stack->items[actual_idx];
}

void *cp_stack_peek(const cp_stack_t *stack) {
    return cp_stack_get(stack, 0);
}

usize cp_stack_len(const cp_stack_t *stack) {
    return stack ? stack->top : 0;
}

bool cp_stack_is_empty(const cp_stack_t *stack) {
    return !stack || stack->top == 0;
}

void cp_stack_clear(cp_stack_t *stack) {
    if (stack) {
        stack->top = 0;
    }
}

/* --- Source: cp_os.c --- */
/**
 * @file cp_os.c
 * @brief Zero-latency Operating System, Process, Hardware, and Environment utilities.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>

i64 cp_os_exec(const char *cmd, char *out_buf, usize out_cap, int *exit_code) {
    if (!cmd) return -1;

    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    usize total_read = 0;
    if (out_buf && out_cap > 0) {
        while (total_read + 1 < out_cap) {
            usize n = fread(out_buf + total_read, 1, out_cap - 1 - total_read, fp);
            if (n == 0) break;
            total_read += n;
        }
        out_buf[total_read] = '\0';

        /* Drain remainder if any */
        char drain[256];
        while (fread(drain, 1, sizeof(drain), fp) > 0) {
            /* discard */
        }
    } else {
        char drain[256];
        while (fread(drain, 1, sizeof(drain), fp) > 0) {
            /* discard */
        }
    }

    int status = pclose(fp);
    if (exit_code) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = 128 + WTERMSIG(status);
        } else {
            *exit_code = status;
        }
    }

    return (i64)total_read;
}

u32 cp_os_num_cpus(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (u32)n : 1;
}

u64 cp_os_total_ram_bytes(void) {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return (u64)pages * (u64)page_size;
    }
    return 0;
}

u64 cp_os_avail_ram_bytes(void) {
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return (u64)pages * (u64)page_size;
    }
    return 0;
}

usize cp_os_page_size(void) {
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (page_size > 0) ? (usize)page_size : 4096;
}

bool cp_os_hostname(char *buf, usize len) {
    if (!buf || len == 0) return false;
    if (gethostname(buf, len) == 0) {
        buf[len - 1] = '\0';
        return true;
    }
    return false;
}

u64 cp_os_uptime_sec(void) {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return (u64)info.uptime;
    }
    return 0;
}

i32 cp_os_pid(void) {
    return (i32)getpid();
}

i32 cp_os_ppid(void) {
    return (i32)getppid();
}

bool cp_os_kill(i32 pid, int sig) {
    return kill((pid_t)pid, sig) == 0;
}

bool cp_os_is_process_running(i32 pid) {
    if (pid <= 0) return false;
    if (kill((pid_t)pid, 0) == 0) return true;
    return (errno == EPERM);
}

const char *cp_os_env_get(const char *name) {
    if (!name) return NULL;
    return getenv(name);
}

const char *cp_os_env_get_or(const char *name, const char *default_val) {
    const char *val = cp_os_env_get(name);
    return val ? val : default_val;
}

i64 cp_os_env_get_int(const char *name, i64 default_val) {
    const char *val = cp_os_env_get(name);
    if (!val || *val == '\0') return default_val;
    char *endptr = NULL;
    long long num = strtoll(val, &endptr, 10);
    if (endptr == val) return default_val;
    return (i64)num;
}

bool cp_os_env_set(const char *name, const char *value) {
    if (!name) return false;
    return setenv(name, value ? value : "", 1) == 0;
}

bool cp_os_env_unset(const char *name) {
    if (!name) return false;
    return unsetenv(name) == 0;
}

bool cp_os_env_has(const char *name) {
    return cp_os_env_get(name) != NULL;
}

void cp_os_sleep_ms(u32 ms) {
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

void cp_os_sleep_us(u64 us) {
    struct timespec req;
    req.tv_sec = (time_t)(us / 1000000ULL);
    req.tv_nsec = (long)(us % 1000000ULL) * 1000L;
    nanosleep(&req, NULL);
}

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_IMPLEMENTATION */
