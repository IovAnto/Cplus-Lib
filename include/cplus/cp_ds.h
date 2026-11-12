/**
 * @file cp_ds.h
 * @brief Zero-latency data structures: Type-safe Vector, Singly Linked List,
 *        Doubly Linked List, and SPSC Lock-Free RingBuffer.
 */

#ifndef CPLUS_DS_H
#define CPLUS_DS_H

#include "cp_core.h"
#include "cp_mem.h"
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

#endif /* CPLUS_DS_H */
