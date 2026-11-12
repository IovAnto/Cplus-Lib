/**
 * @file cp_ds.c
 * @brief High-performance implementation of Vector, Singly/Doubly Linked Lists,
 *        and Lock-Free SPSC RingBuffer.
 */

#include "cplus/cp_ds.h"
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
