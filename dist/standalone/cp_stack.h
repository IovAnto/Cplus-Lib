/**
 * @file cp_stack.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_stack.
 *
 * In ONE file define:
 *   #define CPLUS_CP_STACK_IMPLEMENTATION
 *   #include "cp_stack.h"
 */
#ifndef CPLUS_CP_STACK_H
#define CPLUS_CP_STACK_H

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

#endif /* CPLUS_CP_STACK_H */

#ifdef CPLUS_CP_STACK_IMPLEMENTATION
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
#endif /* CPLUS_CP_STACK_IMPLEMENTATION */
