/**
 * @file cp_mem.h
 * @brief Zero-latency memory architecture: Arena allocator, Scratch pools, 
 *        Fixed-size Pool allocator, and Tracked Heap memory.
 */

#ifndef CPLUS_MEM_H
#define CPLUS_MEM_H

#include "cp_core.h"

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

#endif /* CPLUS_MEM_H */
