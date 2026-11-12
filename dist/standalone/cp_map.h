/**
 * @file cp_map.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_map.
 *
 * In ONE file define:
 *   #define CPLUS_CP_MAP_IMPLEMENTATION
 *   #include "cp_map.h"
 */
#ifndef CPLUS_CP_MAP_H
#define CPLUS_CP_MAP_H

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

#endif /* CPLUS_CP_MAP_H */

#ifdef CPLUS_CP_MAP_IMPLEMENTATION
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
#endif /* CPLUS_CP_MAP_IMPLEMENTATION */
