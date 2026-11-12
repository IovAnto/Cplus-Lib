/**
 * @file bench_mem.c
 * @brief Scientific latency and throughput benchmark: Pure C vs Cplus Memory Architecture.
 *        Executes multiple experimental runs to calculate statistical metrics:
 *        Min, Max, Mean, and Standard Deviation.
 */

#include "cplus/cplus.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_RUNS 5
#define NUM_ALLOCS 1000000
#define BLOCK_SIZE 64

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

static void compute_stats(bench_stats_t *stats) {
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
    stats->latency_ns_op = (stats->mean_ms * 1000000.0) / (f64)NUM_ALLOCS;
}

int main(void) {
    printf("=================================================================================\n");
    printf("  SCIENTIFIC EXPERIMENT REPORT: MEMORY ALLOCATION LATENCY & THROUGHPUT\n");
    printf("  Parameters: %d runs x %d allocations of %d bytes with byte write verification\n",
           NUM_RUNS, NUM_ALLOCS, BLOCK_SIZE);
    printf("=================================================================================\n\n");

    void **ptrs = (void **)malloc(NUM_ALLOCS * sizeof(void *));
    if (!ptrs) {
        fprintf(stderr, "Failed to allocate benchmark pointer table.\n");
        return 1;
    }

    bench_stats_t stats_malloc;
    bench_stats_t stats_arena;
    bench_stats_t stats_pool;

    /* -------------------------------------------------------------------------
     * Test 1: Pure C malloc / free
     * ------------------------------------------------------------------------- */
    printf("[EXPERIMENT 1] Pure C (malloc + write + free individual)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        for (int i = 0; i < NUM_ALLOCS; i++) {
            ptrs[i] = malloc(BLOCK_SIZE);
            ((volatile char *)ptrs[i])[0] = (char)i;
        }
        for (int i = 0; i < NUM_ALLOCS; i++) {
            free(ptrs[i]);
        }
        u64 t1 = get_time_ns();
        stats_malloc.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_malloc.samples_ms[r]);
    }
    compute_stats(&stats_malloc);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n\n",
           stats_malloc.mean_ms, stats_malloc.min_ms, stats_malloc.max_ms,
           stats_malloc.stddev_ms, stats_malloc.latency_ns_op);

    /* -------------------------------------------------------------------------
     * Test 2: Cplus Arena Allocator O(1) + Batch Reset
     * ------------------------------------------------------------------------- */
    printf("[EXPERIMENT 2] Cplus (ArenaAlloc O(1) + write + batch ArenaReset)\n");
    cp_arena_t *arena = ArenaCreate(16 * 1024 * 1024);
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        for (int i = 0; i < NUM_ALLOCS; i++) {
            ptrs[i] = ArenaAlloc(arena, BLOCK_SIZE);
            ((volatile char *)ptrs[i])[0] = (char)i;
        }
        ArenaReset(arena);
        u64 t1 = get_time_ns();
        stats_arena.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_arena.samples_ms[r]);
    }
    ArenaDestroy(arena);
    compute_stats(&stats_arena);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n",
           stats_arena.mean_ms, stats_arena.min_ms, stats_arena.max_ms,
           stats_arena.stddev_ms, stats_arena.latency_ns_op);
    printf("  >> SPEEDUP: %8.2fx FASTER than Pure C\n\n", stats_malloc.mean_ms / stats_arena.mean_ms);

    /* -------------------------------------------------------------------------
     * Test 3: Cplus Fixed-Size Pool Allocator
     * ------------------------------------------------------------------------- */
    printf("[EXPERIMENT 3] Cplus (PoolAlloc O(1) + write + PoolFree O(1))\n");
    cp_pool_t *pool = PoolCreate(BLOCK_SIZE, NUM_ALLOCS);
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        for (int i = 0; i < NUM_ALLOCS; i++) {
            ptrs[i] = PoolAlloc(pool);
            ((volatile char *)ptrs[i])[0] = (char)i;
        }
        for (int i = 0; i < NUM_ALLOCS; i++) {
            PoolFree(pool, ptrs[i]);
        }
        u64 t1 = get_time_ns();
        stats_pool.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_pool.samples_ms[r]);
    }
    PoolDestroy(pool);
    compute_stats(&stats_pool);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n",
           stats_pool.mean_ms, stats_pool.min_ms, stats_pool.max_ms,
           stats_pool.stddev_ms, stats_pool.latency_ns_op);
    printf("  >> SPEEDUP: %8.2fx FASTER than Pure C\n\n", stats_malloc.mean_ms / stats_pool.mean_ms);

    free(ptrs);
    printf("=================================================================================\n");
    return 0;
}
