/**
 * @file cp_mem.c
 * @brief High-performance implementation of Arena, Scratch, Pool, and Tracked Heap allocators.
 */

#include "cplus/cp_mem.h"
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
