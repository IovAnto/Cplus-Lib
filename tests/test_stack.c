/**
 * @file test_stack.c
 * @brief Dedicated unit tests for the Cplus Stack data structure (cp_stack).
 */

#include "cplus/cplus.h"
#include <assert.h>

static void test_stack_basic(void) {
    cp_stack_t *s = StackCreate(4, NULL);
    assert(s != NULL);
    assert(StackIsEmpty(s));
    assert(StackLen(s) == 0);

    /* Push items */
    int a = 10, b = 20, c = 30, d = 40;
    assert(StackPush(s, &a));
    assert(StackPush(s, &b));
    assert(StackPush(s, &c));
    assert(StackPush(s, &d));
    CP_UNUSED(a); CP_UNUSED(b); CP_UNUSED(c); CP_UNUSED(d);

    assert(!StackIsEmpty(s));
    assert(StackLen(s) == 4);

    /* Peek & Get from top */
    assert(StackPeekT(s, int *) == &d);
    assert(*StackPeekT(s, int *) == 40);

    /* Index 0 is top (40), index 1 is 30, index 2 is 20, index 3 is bottom (10) */
    assert(*StackGetT(s, 0, int *) == 40);
    assert(*StackGetT(s, 1, int *) == 30);
    assert(*StackGetT(s, 2, int *) == 20);
    assert(*StackGetT(s, 3, int *) == 10);
    assert(StackGet(s, 4) == NULL);

    /* Pop in LIFO order */
    assert(*StackPopT(s, int *) == 40);
    assert(StackLen(s) == 3);
    assert(*StackPopT(s, int *) == 30);
    assert(*StackPopT(s, int *) == 20);
    assert(*StackPopT(s, int *) == 10);
    assert(StackIsEmpty(s));
    assert(StackPop(s) == NULL);

    StackDestroy(s);
}

static void test_stack_dynamic_resize_and_arena(void) {
    cp_arena_t *arena = ArenaCreate(1024 * 1024);
    assert(arena != NULL);

    cp_stack_t *s = stack.create(2, arena);
    assert(s != NULL);

    /* Push 500 integers causing multiple resizes */
    for (intptr_t i = 0; i < 500; i++) {
        bool ok = stack.push(s, (void *)i);
        assert(ok);
        CP_UNUSED(ok);
    }
    assert(stack.len(s) == 500);
    assert((intptr_t)stack.peek(s) == 499);
    assert((intptr_t)stack.get(s, 0) == 499);
    assert((intptr_t)stack.get(s, 499) == 0);

    /* Verify popping all */
    for (intptr_t i = 499; i >= 0; i--) {
        intptr_t val = (intptr_t)stack.pop(s);
        assert(val == i);
        CP_UNUSED(val);
    }
    assert(stack.isEmpty(s));

    ArenaDestroy(arena);
}

static void test_stack_clear_and_empty(void) {
    cp_stack_t *s = StackCreate(8, NULL);
    assert(StackIsEmpty(s));
    for (intptr_t i = 0; i < 5; i++) {
        StackPush(s, (void *)i);
    }
    assert(StackLen(s) == 5);
    StackClear(s);
    assert(StackLen(s) == 0);
    assert(StackIsEmpty(s));
    assert(StackPop(s) == NULL);
    assert(StackPeek(s) == NULL);
    assert(StackGet(s, 0) == NULL);
    StackDestroy(s);
}

int main(void) {
    printf("Running test_stack...\n");
    test_stack_basic();
    test_stack_dynamic_resize_and_arena();
    test_stack_clear_and_empty();
    printf("All test_stack unit tests passed successfully!\n");
    return 0;
}
