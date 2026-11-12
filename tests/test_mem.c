/**
 * @file test_mem.c
 * @brief Unit tests for cp_core and cp_mem modules.
 */

#include "cplus/cplus.h"
#include <assert.h>
#include <string.h>

static int g_defer_counter = 0;

static void increment_counter(void *arg) {
    int delta = *(int *)arg;
    g_defer_counter += delta;
}

static void increment_action(void) {
    g_defer_counter += 100;
}

static void test_core_types_and_defer(void) {
    printf("[TEST] Core types and RAII Defer...\n");

    /* Type sizes */
    assert(sizeof(u8) == 1);
    assert(sizeof(u16) == 2);
    assert(sizeof(u32) == 4);
    assert(sizeof(u64) == 8);
    assert(sizeof(i32) == 4);
    assert(sizeof(i64) == 8);
    assert(sizeof(f32) == 4);
    assert(sizeof(f64) == 8);

    /* Test Defer scoping */
    g_defer_counter = 0;
    int val1 = 5;
    int val2 = 10;
    {
        Defer(increment_counter, &val1);
        Defer(increment_counter, &val2);
        DeferAction(increment_action);
        assert(g_defer_counter == 0);
    }
    /* Action (+100), val2 (+10), val1 (+5) -> 115 */
    assert(g_defer_counter == 115);

    printf("  -> Core and Defer tests PASSED.\n");
}

static void test_arena_allocator(void) {
    printf("[TEST] Arena Allocator (PascalCase & Dot-Notation)...\n");

    /* PascalCase */
    cp_arena_t *a = ArenaCreate(4096);
    assert(a != NULL);

    int *arr1 = (int *)ArenaAlloc(a, 100 * sizeof(int));
    assert(arr1 != NULL);
    assert(((uintptr_t)arr1 % CP_DEFAULT_ALIGNMENT) == 0);

    /* Test 32-byte SIMD alignment */
    float *simd_arr = (float *)ArenaAllocAlign(a, 64 * sizeof(float), CP_SIMD_ALIGNMENT);
    assert(simd_arr != NULL);
    assert(((uintptr_t)simd_arr % CP_SIMD_ALIGNMENT) == 0);
    CP_UNUSED(simd_arr);

    /* Fill memory */
    for (int i = 0; i < 100; i++) arr1[i] = i * 2;
    for (int i = 0; i < 100; i++) assert(arr1[i] == i * 2);

    /* Test ArenaAllocZero */
    char *zero_buf = (char *)ArenaAllocZero(a, 128);
    assert(zero_buf != NULL);
    for (int i = 0; i < 128; i++) assert(zero_buf[i] == 0);
    CP_UNUSED(zero_buf);

    /* Temp Arena Scopes */
    cp_temp_arena_t temp = TempBegin(a);
    void *temp_mem = ArenaAlloc(a, 1024);
    assert(temp_mem != NULL);
    TempEnd(temp);

    /* Check that new allocation reuses the temp offset */
    void *reused_mem = ArenaAlloc(a, 512);
    assert(reused_mem == temp_mem);
    CP_UNUSED(temp_mem);
    CP_UNUSED(reused_mem);

    ArenaReset(a);
    assert(a->total_allocated == 0);
    ArenaDestroy(a);

    /* Dot Notation test */
    cp_arena_t *a2 = arena.create(8192);
    void *p_dot = arena.alloc(a2, 256);
    assert(p_dot != NULL);
    assert(((uintptr_t)p_dot % CP_DEFAULT_ALIGNMENT) == 0);
    CP_UNUSED(p_dot);
    arena.destroy(a2);

    printf("  -> Arena tests PASSED.\n");
}

static void test_scratch_arena(void) {
    printf("[TEST] Scratch Arena...\n");

    cp_arena_t *sc1 = Scratch();
    assert(sc1 != NULL);
    CP_UNUSED(sc1);

    cp_temp_arena_t t1 = ScratchBegin();
    void *ptr1 = ArenaAlloc(t1.arena, 128);
    assert(ptr1 != NULL);
    CP_UNUSED(ptr1);

    /* Nested Scratch call gets the alternate scratch arena */
    cp_temp_arena_t t2 = ScratchBegin();
    assert(t2.arena != t1.arena);
    void *ptr2 = ArenaAlloc(t2.arena, 256);
    assert(ptr2 != NULL);
    CP_UNUSED(ptr2);
    ScratchEnd(t2);

    ScratchEnd(t1);

    printf("  -> Scratch Arena tests PASSED.\n");
}

static void test_pool_allocator(void) {
    printf("[TEST] Fixed-Size Pool Allocator...\n");

    /* Create pool for 50 elements of 32 bytes */
    cp_pool_t *p = PoolCreate(32, 50);
    assert(p != NULL);
    assert(p->item_count == 50);

    void *ptrs[50];
    for (int i = 0; i < 50; i++) {
        ptrs[i] = PoolAlloc(p);
        assert(ptrs[i] != NULL);
        memset(ptrs[i], 0xAA, 32);
    }
    assert(p->used == 50);
    assert(PoolAlloc(p) == NULL); /* Pool exhausted */

    /* Free some and re-alloc */
    PoolFree(p, ptrs[10]);
    PoolFree(p, ptrs[20]);
    assert(p->used == 48);

    void *new_p1 = PoolAlloc(p);
    void *new_p2 = PoolAlloc(p);
    assert(new_p1 != NULL && new_p2 != NULL);
    CP_UNUSED(new_p1);
    CP_UNUSED(new_p2);
    assert(p->used == 50);

    PoolDestroy(p);

    /* Dot notation */
    cp_pool_t *p2 = pool.create(64, 10);
    void *item = pool.alloc(p2);
    assert(item != NULL);
    pool.free(p2, item);
    pool.destroy(p2);

    printf("  -> Pool Allocator tests PASSED.\n");
}

static void test_heap_tracker(void) {
    printf("[TEST] Tracked Heap Allocator...\n");

    assert(MemActiveLeaks() == 0);

    void *block1 = MemAlloc(128);
    void *block2 = MemAlloc(256);
    assert(block1 != NULL && block2 != NULL);
    assert(MemActiveLeaks() == 2);

    /* Realloc */
    block1 = MemRealloc(block1, 512);
    assert(block1 != NULL);
    assert(MemActiveLeaks() == 2);

    MemFree(block1);
    assert(MemActiveLeaks() == 1);

    MemFree(block2);
    assert(MemActiveLeaks() == 0);

    /* Dot notation */
    void *b3 = mem.alloc(64);
    assert(mem.activeLeaks() == 1);
    mem.free(b3);
    assert(mem.activeLeaks() == 0);

    MemReportLeaks();
    printf("  -> Tracked Heap tests PASSED.\n");
}

int main(void) {
    printf("\n=== RUNNING CPLUS CORE & MEMORY TESTS ===\n");
    test_core_types_and_defer();
    test_arena_allocator();
    test_scratch_arena();
    test_pool_allocator();
    test_heap_tracker();
    printf("=== ALL UNIT TESTS PASSED SUCCESSFULLY! ===\n\n");
    return 0;
}
