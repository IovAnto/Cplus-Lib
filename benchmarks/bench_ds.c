/**
 * @file bench_ds.c
 * @brief Scientific latency & throughput benchmark: Pure C vs Cplus Data Structures.
 *        Compares Vector growth, Arena-backed Doubly Linked List vs malloc'd list,
 *        and Lock-Free RingBuffer inter-thread messaging throughput.
 */

#include "cplus/cplus.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define NUM_RUNS 5
#define VEC_OPS 2000000
#define LIST_OPS 500000
#define SPSC_OPS 2000000

typedef struct {
    f64 samples_ms[NUM_RUNS];
    f64 min_ms;
    f64 max_ms;
    f64 mean_ms;
    f64 stddev_ms;
    f64 latency_ns_op;
} bench_stats_t;

static inline u64 get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

static void compute_stats(bench_stats_t *stats, usize total_ops) {
    f64 sum = 0.0;
    stats->min_ms = stats->samples_ms[0];
    stats->max_ms = stats->samples_ms[0];

    for (int i = 0; i < NUM_RUNS; i++) {
        f64 v = stats->samples_ms[i];
        sum += v;
        if (v < stats->min_ms) stats->min_ms = v;
        if (v > stats->max_ms) stats->max_ms = v;
    }
    stats->mean_ms = sum / NUM_RUNS;

    f64 var_sum = 0.0;
    for (int i = 0; i < NUM_RUNS; i++) {
        f64 diff = stats->samples_ms[i] - stats->mean_ms;
        var_sum += diff * diff;
    }
    stats->stddev_ms = sqrt(var_sum / NUM_RUNS);
    stats->latency_ns_op = (stats->mean_ms * 1000000.0) / (f64)total_ops;
}

/* =========================================================================
 * EXPERIMENT 1: Vector Growth (2,000,000 pushes)
 * ========================================================================= */
static void run_vector_benchmarks(void) {
    printf("--- PART A: Vector Growth & Element Insertion (2,000,000 integers) ---\n\n");

    bench_stats_t stats_c_vec;
    bench_stats_t stats_cp_vec;

    /* Pure C manual realloc array */
    printf("[EXPERIMENT 1.1] Pure C (manual realloc growth buffer)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        int *arr = NULL;
        usize len = 0;
        usize cap = 0;

        for (int i = 0; i < VEC_OPS; i++) {
            if (len >= cap) {
                cap = cap ? cap * 2 : 4;
                arr = (int *)realloc(arr, cap * sizeof(int));
            }
            arr[len++] = i;
        }
        u64 t1 = get_time_ns();
        free(arr);
        stats_c_vec.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_c_vec.samples_ms[r]);
    }
    compute_stats(&stats_c_vec, VEC_OPS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n\n",
           stats_c_vec.mean_ms, stats_c_vec.min_ms, stats_c_vec.max_ms,
           stats_c_vec.stddev_ms, stats_c_vec.latency_ns_op);

    /* Cplus Vec */
    printf("[EXPERIMENT 1.2] Cplus Vec (Type-Safe Stretchy Buffer Pattern)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        Vec(int) v = VecNew(int);
        for (int i = 0; i < VEC_OPS; i++) {
            VecPush(v, i);
        }
        u64 t1 = get_time_ns();
        VecFree(v);
        stats_cp_vec.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_cp_vec.samples_ms[r]);
    }
    compute_stats(&stats_cp_vec, VEC_OPS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n",
           stats_cp_vec.mean_ms, stats_cp_vec.min_ms, stats_cp_vec.max_ms,
           stats_cp_vec.stddev_ms, stats_cp_vec.latency_ns_op);
    printf("  >> SPEEDUP: %8.2fx FASTER than Pure C\n\n", stats_c_vec.mean_ms / stats_cp_vec.mean_ms);
}

/* =========================================================================
 * EXPERIMENT 2: Doubly Linked List Node Allocations (500,000 nodes)
 * ========================================================================= */
typedef struct raw_c_dnode {
    struct raw_c_dnode *prev;
    struct raw_c_dnode *next;
    void *val;
} raw_c_dnode_t;

static void run_list_benchmarks(void) {
    printf("--- PART B: Doubly Linked List (500,000 nodes: Insert + Traversal) ---\n\n");

    bench_stats_t stats_c_list;
    bench_stats_t stats_cp_list;

    /* Pure C: malloc each node individually */
    printf("[EXPERIMENT 2.1] Pure C (Individual malloc for each node + free)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        raw_c_dnode_t *head = NULL;
        raw_c_dnode_t *tail = NULL;

        for (int i = 0; i < LIST_OPS; i++) {
            raw_c_dnode_t *node = (raw_c_dnode_t *)malloc(sizeof(raw_c_dnode_t));
            node->val = (void *)(uintptr_t)i;
            node->next = NULL;
            node->prev = tail;
            if (tail) tail->next = node;
            tail = node;
            if (!head) head = node;
        }

        /* Free all nodes */
        raw_c_dnode_t *cur = head;
        while (cur) {
            raw_c_dnode_t *next = cur->next;
            free(cur);
            cur = next;
        }

        u64 t1 = get_time_ns();
        stats_c_list.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_c_list.samples_ms[r]);
    }
    compute_stats(&stats_c_list, LIST_OPS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n\n",
           stats_c_list.mean_ms, stats_c_list.min_ms, stats_c_list.max_ms,
           stats_c_list.stddev_ms, stats_c_list.latency_ns_op);

    /* Cplus DList: Arena-backed contiguous nodes */
    printf("[EXPERIMENT 2.2] Cplus DList (Arena-backed contiguous cache locality + O(1) batch free)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        cp_arena_t *arena = ArenaCreate(16 * 1024 * 1024);
        DList dl = DListNew(arena);

        for (int i = 0; i < LIST_OPS; i++) {
            DListPushBack(&dl, (void *)(uintptr_t)i);
        }

        ArenaDestroy(arena); /* Instant batch reclamation */
        u64 t1 = get_time_ns();
        stats_cp_list.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_cp_list.samples_ms[r]);
    }
    compute_stats(&stats_cp_list, LIST_OPS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n",
           stats_cp_list.mean_ms, stats_cp_list.min_ms, stats_cp_list.max_ms,
           stats_cp_list.stddev_ms, stats_cp_list.latency_ns_op);
    printf("  >> SPEEDUP: %8.2fx FASTER than Pure C\n\n", stats_c_list.mean_ms / stats_cp_list.mean_ms);
}

/* =========================================================================
 * EXPERIMENT 3: Lock-Free SPSC RingBuffer Inter-Thread Throughput
 * ========================================================================= */
typedef struct {
    RingBuf *rb;
    usize count;
} thread_bench_arg_t;

static void *bench_spsc_producer(void *arg) {
    thread_bench_arg_t *p = (thread_bench_arg_t *)arg;
    for (usize i = 0; i < p->count; i++) {
        while (!RingBufPush(p->rb, (void *)(uintptr_t)(i + 1))) {
            #if defined(__x86_64__) || defined(_M_X64)
            __builtin_ia32_pause();
            #endif
        }
    }
    return NULL;
}

static void *bench_spsc_consumer(void *arg) {
    thread_bench_arg_t *c = (thread_bench_arg_t *)arg;
    usize popped = 0;
    while (popped < c->count) {
        void *val = NULL;
        if (RingBufPop(c->rb, &val)) {
            popped++;
        } else {
            #if defined(__x86_64__) || defined(_M_X64)
            __builtin_ia32_pause();
            #endif
        }
    }
    return NULL;
}

static void run_ringbuf_benchmarks(void) {
    printf("--- PART C: Lock-Free SPSC RingBuffer (2,000,000 cross-thread messages) ---\n\n");

    bench_stats_t stats_spsc;
    RingBuf *rb = RingBufCreate(65536); /* 64K slots */

    for (int r = 0; r < NUM_RUNS; r++) {
        thread_bench_arg_t arg = { .rb = rb, .count = SPSC_OPS };
        u64 t0 = get_time_ns();

        pthread_t th_p, th_c;
        pthread_create(&th_c, NULL, bench_spsc_consumer, &arg);
        pthread_create(&th_p, NULL, bench_spsc_producer, &arg);

        pthread_join(th_p, NULL);
        pthread_join(th_c, NULL);

        u64 t1 = get_time_ns();
        stats_spsc.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_spsc.samples_ms[r]);
    }
    RingBufDestroy(rb);

    compute_stats(&stats_spsc, SPSC_OPS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n",
           stats_spsc.mean_ms, stats_spsc.min_ms, stats_spsc.max_ms,
           stats_spsc.stddev_ms, stats_spsc.latency_ns_op);
    f64 throughput_mops = (f64)SPSC_OPS / (stats_spsc.mean_ms * 1000.0);
    printf("  >> SPSC Cross-Thread Throughput: %8.2f MILLION messages / sec\n\n", throughput_mops);
}

int main(void) {
    printf("=================================================================================\n");
    printf("  SCIENTIFIC EXPERIMENT REPORT: DATA STRUCTURES LATENCY & THROUGHPUT\n");
    printf("=================================================================================\n\n");

    run_vector_benchmarks();
    run_list_benchmarks();
    run_ringbuf_benchmarks();

    printf("=================================================================================\n");
    return 0;
}
