/**
 * @file bench_stack.c
 * @brief Statistical benchmark for Stack Push and Pop operations.
 */

#define _POSIX_C_SOURCE 199309L
#include "cplus/cplus.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define RUNS 5
#define STACK_OPS 10000000

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

typedef struct {
    double times[RUNS];
    double mean;
    double min;
    double max;
    double stddev;
} stats_t;

static stats_t calc_stats(double *times, int n) {
    stats_t s;
    s.min = times[0];
    s.max = times[0];
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        s.times[i] = times[i];
        if (times[i] < s.min) s.min = times[i];
        if (times[i] > s.max) s.max = times[i];
        sum += times[i];
    }
    s.mean = sum / n;
    double var = 0.0;
    for (int i = 0; i < n; i++) {
        var += (times[i] - s.mean) * (times[i] - s.mean);
    }
    s.stddev = sqrt(var / n);
    return s;
}

typedef struct {
    void **items;
    size_t cap;
    size_t top;
} c_raw_stack_t;

static void c_stack_push(c_raw_stack_t *s, void *item) {
    if (s->top >= s->cap) {
        s->cap *= 2;
        s->items = (void **)realloc(s->items, s->cap * sizeof(void *));
    }
    s->items[s->top++] = item;
}

static void *c_stack_pop(c_raw_stack_t *s) {
    if (s->top == 0) return NULL;
    return s->items[--s->top];
}

int main(void) {
    printf("===================================================================\n");
    printf("  Cplus Stack Benchmark Suite (%d operations, N=%d)\n", STACK_OPS * 2, RUNS);
    printf("===================================================================\n");

    double c_times[RUNS];
    double cp_times[RUNS];

    /* Baseline: Pure C stack */
    for (int r = 0; r < RUNS; r++) {
        c_raw_stack_t s;
        s.cap = STACK_OPS + 16;
        s.top = 0;
        s.items = (void **)malloc(s.cap * sizeof(void *));

        double t0 = get_time_sec();
        for (intptr_t i = 0; i < STACK_OPS; i++) {
            c_stack_push(&s, (void *)i);
        }
        intptr_t sum = 0;
        for (intptr_t i = 0; i < STACK_OPS; i++) {
            sum += (intptr_t)c_stack_pop(&s);
        }
        __asm__ volatile("" : : "r"(sum) : "memory");
        double t1 = get_time_sec();
        c_times[r] = (t1 - t0) * 1000.0;
        free(s.items);
    }

    /* Cplus Arena-backed Stack */
    cp_arena_t *arena = ArenaCreate(STACK_OPS * sizeof(void *) + 1024 * 1024);
    for (int r = 0; r < RUNS; r++) {
        cp_stack_t *s = StackCreate(STACK_OPS + 16, arena);

        double t0 = get_time_sec();
        for (intptr_t i = 0; i < STACK_OPS; i++) {
            StackPush(s, (void *)i);
        }
        intptr_t sum = 0;
        for (intptr_t i = 0; i < STACK_OPS; i++) {
            sum += (intptr_t)StackPop(s);
        }
        __asm__ volatile("" : : "r"(sum) : "memory");
        double t1 = get_time_sec();
        cp_times[r] = (t1 - t0) * 1000.0;
        ArenaReset(arena);
    }
    ArenaDestroy(arena);

    stats_t sc = calc_stats(c_times, RUNS);
    stats_t scp = calc_stats(cp_times, RUNS);

    printf("Pure C Stack (malloc):  Mean: %7.2f ms (Min: %7.2f, Max: %7.2f, StdDev: %5.2f ms) | %7.2f M ops/s\n",
           sc.mean, sc.min, sc.max, sc.stddev, (STACK_OPS * 2.0 / 1e6) / (sc.mean / 1000.0));
    printf("Cplus Arena Stack:      Mean: %7.2f ms (Min: %7.2f, Max: %7.2f, StdDev: %5.2f ms) | %7.2f M ops/s\n",
           scp.mean, scp.min, scp.max, scp.stddev, (STACK_OPS * 2.0 / 1e6) / (scp.mean / 1000.0));
    printf("Relative speed: %.2fx (Throughput: %.2f M ops/s, %.2f ns/op)\n",
           sc.mean / scp.mean, (STACK_OPS * 2.0 / 1e6) / (scp.mean / 1000.0), (scp.mean * 1e6) / (STACK_OPS * 2.0));
    return 0;
}
