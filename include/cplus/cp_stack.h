/**
 * @file cp_stack.h
 * @brief High-performance L1 cache-friendly Stack data structure with push, pop, and index-from-top get.
 */

#ifndef CPLUS_STACK_H
#define CPLUS_STACK_H

#include "cp_core.h"
#include "cp_mem.h"

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

#endif /* CPLUS_STACK_H */
