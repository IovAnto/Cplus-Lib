/**
 * @file cp_stack.c
 * @brief High-performance L1 cache-friendly Stack implementation.
 */

#include "cplus/cp_stack.h"
#include "cplus/cp_mem.h"
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
