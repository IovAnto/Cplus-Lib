/**
 * @file cp_mem.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_mem.
 *
 * In ONE file define:
 *   #define CPLUS_CP_MEM_IMPLEMENTATION
 *   #include "cp_mem.h"
 */
#ifndef CPLUS_CP_MEM_H
#define CPLUS_CP_MEM_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <pthread.h>
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

#endif /* CPLUS_CP_MEM_H */

#ifdef CPLUS_CP_MEM_IMPLEMENTATION
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
#endif /* CPLUS_CP_MEM_IMPLEMENTATION */
