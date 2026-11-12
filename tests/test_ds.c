/**
 * @file test_ds.c
 * @brief Unit tests for cp_ds: Vector, Singly/Doubly Linked Lists, and SPSC RingBuffer.
 */

#include "cplus/cplus.h"
#include <assert.h>
#include <pthread.h>

static void test_dynamic_vector(void) {
    printf("[TEST] Dynamic Vector (PascalCase & Dot-Notation)...\n");

    Vec(int) v = VecNew(int);
    assert(VecLen(v) == 0);
    assert(vec.len(v) == 0);

    /* Push elements */
    for (int i = 0; i < 1000; i++) {
        VecPush(v, i * 3);
    }
    assert(VecLen(v) == 1000);
    assert(vec.len(v) == 1000);
    assert(VecCap(v) >= 1000);

    /* Direct array indexing without overhead */
    for (int i = 0; i < 1000; i++) {
        assert(v[i] == i * 3);
    }

    /* Pop */
    VecPop(v);
    assert(VecLen(v) == 999);
    assert(v[998] == 998 * 3);

    /* Functional iterators: VecEach and VecMap */
    Vec(int) numbers = VecNew(int);
    for (int i = 1; i <= 5; i++) {
        VecPush(numbers, i);
    }
    int sum = 0;
    VecEach(numbers, idx, val, {
        sum += val;
        CP_UNUSED(idx);
    });
    assert(sum == 15);
    CP_UNUSED(sum);

    /* In-place map: x -> x * 10 */
    VecMap(numbers, item, {
        *item *= 10;
    });
    assert(numbers[0] == 10);
    assert(numbers[1] == 20);
    assert(numbers[2] == 30);
    assert(numbers[3] == 40);
    assert(numbers[4] == 50);
    VecFree(numbers);

    VecClear(v);
    assert(VecLen(v) == 0);

    VecFree(v);
    assert(v == NULL);

    printf("  -> Dynamic Vector tests PASSED.\n");
}

static void test_singly_linked_list(void) {
    printf("[TEST] Singly Linked List...\n");

    cp_arena_t *a = ArenaCreate(4096);
    SList l = ListNew(a);
    assert(ListLen(&l) == 0);

    /* Push front and back */
    ListPushBack(&l, (void *)(uintptr_t)10);
    ListPushBack(&l, (void *)(uintptr_t)20);
    ListPushFront(&l, (void *)(uintptr_t)5);
    assert(ListLen(&l) == 3);

    /* Pop front */
    void *v1 = ListPopFront(&l);
    assert((uintptr_t)v1 == 5);
    CP_UNUSED(v1);

    void *v2 = ListPopFront(&l);
    assert((uintptr_t)v2 == 10);
    CP_UNUSED(v2);

    void *v3 = ListPopFront(&l);
    assert((uintptr_t)v3 == 20);
    CP_UNUSED(v3);

    assert(ListPopFront(&l) == NULL);
    assert(ListLen(&l) == 0);

    /* Dot notation test */
    SList l2 = list.create(a);
    list.pushFront(&l2, (void *)(uintptr_t)99);
    assert(list.len(&l2) == 1);
    void *v_dot = list.popFront(&l2);
    assert((uintptr_t)v_dot == 99);
    CP_UNUSED(v_dot);

    ArenaDestroy(a);
    printf("  -> Singly Linked List tests PASSED.\n");
}

static void test_doubly_linked_list(void) {
    printf("[TEST] Doubly Linked List...\n");

    cp_arena_t *a = ArenaCreate(4096);
    DList dl = DListNew(a);
    assert(DListLen(&dl) == 0);

    cp_dnode_t *n1 = DListPushBack(&dl, (void *)(uintptr_t)100);
    cp_dnode_t *n2 = DListPushBack(&dl, (void *)(uintptr_t)200);
    cp_dnode_t *n3 = DListPushBack(&dl, (void *)(uintptr_t)300);
    cp_dnode_t *n0 = DListPushFront(&dl, (void *)(uintptr_t)50);
    assert(DListLen(&dl) == 4);

    /* Insert after */
    cp_dnode_t *n_mid = DListInsertAfter(&dl, n1, (void *)(uintptr_t)150);
    assert(DListLen(&dl) == 5);

    /* Remove node in O(1) */
    DListRemove(&dl, n2);
    assert(DListLen(&dl) == 4);

    /* Verify order: 50 -> 100 -> 150 -> 300 */
    assert((uintptr_t)DListPopFront(&dl) == 50);
    assert((uintptr_t)DListPopFront(&dl) == 100);
    assert((uintptr_t)DListPopFront(&dl) == 150);
    assert((uintptr_t)DListPopFront(&dl) == 300);
    assert(DListLen(&dl) == 0);

    CP_UNUSED(n0);
    CP_UNUSED(n1);
    CP_UNUSED(n3);
    CP_UNUSED(n_mid);

    /* Dot notation */
    DList dl2 = dlist.create(a);
    dlist.pushBack(&dl2, (void *)(uintptr_t)777);
    assert(dlist.len(&dl2) == 1);
    assert((uintptr_t)dlist.popBack(&dl2) == 777);

    ArenaDestroy(a);
    printf("  -> Doubly Linked List tests PASSED.\n");
}

#define SPSC_THREAD_ITEMS 100000

typedef struct {
    RingBuf *rb;
    u64 sum_produced;
} producer_arg_t;

typedef struct {
    RingBuf *rb;
    u64 sum_consumed;
    usize count_consumed;
} consumer_arg_t;

static void *spsc_producer_thread(void *arg) {
    producer_arg_t *p = (producer_arg_t *)arg;
    for (usize i = 1; i <= SPSC_THREAD_ITEMS; i++) {
        while (!RingBufPush(p->rb, (void *)(uintptr_t)i)) {
            /* Spin / yield until space available */
            sched_yield();
        }
        p->sum_produced += i;
    }
    return NULL;
}

static void *spsc_consumer_thread(void *arg) {
    consumer_arg_t *c = (consumer_arg_t *)arg;
    while (c->count_consumed < SPSC_THREAD_ITEMS) {
        void *val = NULL;
        if (RingBufPop(c->rb, &val)) {
            c->sum_consumed += (uintptr_t)val;
            c->count_consumed++;
        } else {
            sched_yield();
        }
    }
    return NULL;
}

static void test_lock_free_spsc_ringbuffer(void) {
    printf("[TEST] Lock-Free SPSC RingBuffer (Single & Multi-Thread)...\n");

    /* Single thread FIFO test */
    RingBuf *rb = RingBufCreate(8);
    assert(rb != NULL);

    for (int i = 0; i < 8; i++) {
        assert(RingBufPush(rb, (void *)(uintptr_t)(i + 1)) == true);
    }
    /* Capacity limit reached (8/8) */
    assert(RingBufPush(rb, (void *)(uintptr_t)99) == false);

    for (int i = 0; i < 8; i++) {
        void *item = NULL;
        assert(RingBufPop(rb, &item) == true);
        assert((uintptr_t)item == (uintptr_t)(i + 1));
        CP_UNUSED(item);
    }
    assert(RingBufCount(rb) == 0);

    /* Multithreaded SPSC thread exchange */
    producer_arg_t prod_arg = { .rb = rb, .sum_produced = 0 };
    consumer_arg_t cons_arg = { .rb = rb, .sum_consumed = 0, .count_consumed = 0 };

    pthread_t th_prod, th_cons;
    pthread_create(&th_cons, NULL, spsc_consumer_thread, &cons_arg);
    pthread_create(&th_prod, NULL, spsc_producer_thread, &prod_arg);

    pthread_join(th_prod, NULL);
    pthread_join(th_cons, NULL);

    assert(cons_arg.count_consumed == SPSC_THREAD_ITEMS);
    assert(cons_arg.sum_consumed == prod_arg.sum_produced);

    RingBufDestroy(rb);
    printf("  -> Lock-Free SPSC RingBuffer tests PASSED.\n");
}

int main(void) {
    printf("\n=== RUNNING CPLUS DATA STRUCTURES TESTS ===\n");
    test_dynamic_vector();
    test_singly_linked_list();
    test_doubly_linked_list();
    test_lock_free_spsc_ringbuffer();
    printf("=== ALL UNIT TESTS PASSED SUCCESSFULLY! ===\n\n");
    return 0;
}
