/**
 * @file cp_ds.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_ds.
 *
 * In ONE file define:
 *   #define CPLUS_CP_DS_IMPLEMENTATION
 *   #include "cp_ds.h"
 */
#ifndef CPLUS_CP_DS_H
#define CPLUS_CP_DS_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <stdatomic.h>
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

#endif /* CPLUS_CP_DS_H */

#ifdef CPLUS_CP_DS_IMPLEMENTATION
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
#endif /* CPLUS_CP_DS_IMPLEMENTATION */
