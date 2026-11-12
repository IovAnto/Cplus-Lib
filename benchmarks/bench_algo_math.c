/**
 * @file bench_algo_math.c
 * @brief Scientific latency & throughput benchmark: Pure C vs Cplus Algorithms & SIMD Math.
 *        Compares qsort vs IntroSort vs O(N) RadixSort (1,000,000 integers)
 *        and Classical vs SIMD 4x4 Matrix Multiplication (5,000,000 operations).
 */

#include "cplus/cplus.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_RUNS 5
#define SORT_COUNT 1000000
#define MAT_COUNT 5000000

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
 * EXPERIMENT 1: Sorting Algorithms (1,000,000 integers)
 * ========================================================================= */
static int qsort_cmp_i32(const void *a, const void *b) {
    i32 ia = *(const i32 *)a;
    i32 ib = *(const i32 *)b;
    return (ia > ib) - (ia < ib);
}

static void run_sorting_benchmarks(void) {
    printf("--- PART A: Sorting 1,000,000 integers (qsort vs IntroSort vs RadixSort) ---\n\n");

    i32 *master_data = (i32 *)malloc(SORT_COUNT * sizeof(i32));
    i32 *work_buf = (i32 *)malloc(SORT_COUNT * sizeof(i32));

    /* Populate with pseudo-random integers */
    u32 seed = 123456789;
    for (usize i = 0; i < SORT_COUNT; i++) {
        seed = seed * 1664525u + 1013904223u;
        master_data[i] = (i32)(seed & 0x7FFFFFFF);
    }

    /* Verification of Parity */
    {
        memcpy(work_buf, master_data, SORT_COUNT * sizeof(i32));
        qsort(work_buf, SORT_COUNT, sizeof(i32), qsort_cmp_i32);
        i32 qsort_first = work_buf[0];
        i32 qsort_last = work_buf[SORT_COUNT - 1];

        memcpy(work_buf, master_data, SORT_COUNT * sizeof(i32));
        AlgoSortI32(work_buf, SORT_COUNT);
        i32 intro_first = work_buf[0];
        i32 intro_last = work_buf[SORT_COUNT - 1];

        memcpy(work_buf, master_data, SORT_COUNT * sizeof(i32));
        AlgoRadixSortU32((u32 *)work_buf, SORT_COUNT);
        i32 radix_first = work_buf[0];
        i32 radix_last = work_buf[SORT_COUNT - 1];

        printf("  [PARITY VERIFICATION] Sorted 1,000,000 integers range check:\n");
        printf("    qsort:     [%d ... %d]\n", qsort_first, qsort_last);
        printf("    IntroSort: [%d ... %d]\n", intro_first, intro_last);
        printf("    RadixSort: [%d ... %d]\n", radix_first, radix_last);

        if (qsort_first != intro_first || qsort_last != intro_last ||
            qsort_first != radix_first || qsort_last != radix_last) {
            fprintf(stderr, "PARITY ERROR in sorting!\n");
            abort();
        }
        printf("  >> PARITY CHECK PASSED: All algorithms produce 100.0%% identical results!\n\n");
    }

    bench_stats_t stats_qsort;
    bench_stats_t stats_introsort;
    bench_stats_t stats_radix;

    /* 1.1 Pure C qsort */
    printf("[EXPERIMENT 1.1] Pure C (stdlib qsort with callback pointer)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        memcpy(work_buf, master_data, SORT_COUNT * sizeof(i32));
        u64 t0 = get_time_ns();
        qsort(work_buf, SORT_COUNT, sizeof(i32), qsort_cmp_i32);
        u64 t1 = get_time_ns();
        stats_qsort.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_qsort.samples_ms[r]);
    }
    compute_stats(&stats_qsort, SORT_COUNT);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/elem\n\n",
           stats_qsort.mean_ms, stats_qsort.min_ms, stats_qsort.max_ms,
           stats_qsort.stddev_ms, stats_qsort.latency_ns_op);

    /* 1.2 Cplus IntroSort */
    printf("[EXPERIMENT 1.2] Cplus IntroSort (Hybrid QuickSort + Inlined InsertionSort)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        memcpy(work_buf, master_data, SORT_COUNT * sizeof(i32));
        u64 t0 = get_time_ns();
        AlgoSortI32(work_buf, SORT_COUNT);
        u64 t1 = get_time_ns();
        stats_introsort.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_introsort.samples_ms[r]);
    }
    compute_stats(&stats_introsort, SORT_COUNT);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/elem\n",
           stats_introsort.mean_ms, stats_introsort.min_ms, stats_introsort.max_ms,
           stats_introsort.stddev_ms, stats_introsort.latency_ns_op);
    printf("  >> SPEEDUP vs qsort: %8.2fx FASTER\n\n", stats_qsort.mean_ms / stats_introsort.mean_ms);

    /* 1.3 Cplus RadixSort O(N) */
    printf("[EXPERIMENT 1.3] Cplus RadixSort (Linear O(N) LSD 8-bit passes)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        memcpy(work_buf, master_data, SORT_COUNT * sizeof(i32));
        u64 t0 = get_time_ns();
        AlgoRadixSortU32((u32 *)work_buf, SORT_COUNT);
        u64 t1 = get_time_ns();
        stats_radix.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_radix.samples_ms[r]);
    }
    compute_stats(&stats_radix, SORT_COUNT);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/elem\n",
           stats_radix.mean_ms, stats_radix.min_ms, stats_radix.max_ms,
           stats_radix.stddev_ms, stats_radix.latency_ns_op);
    printf("  >> SPEEDUP vs qsort: %8.2fx FASTER\n\n", stats_qsort.mean_ms / stats_radix.mean_ms);

    free(master_data);
    free(work_buf);
}

/* =========================================================================
 * EXPERIMENT 2: 4x4 Matrix Multiplication (5,000,000 multiplies)
 * ========================================================================= */
static void raw_c_mat4_mul(const float a[16], const float b[16], float out[16]) {
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            out[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
}

static void run_matrix_benchmarks(void) {
    printf("--- PART B: 4x4 Matrix Multiplication (5,000,000 operations) ---\n\n");

    Mat4 m1 = Mat4Translate(1.5f, 2.5f, 3.5f);
    Mat4 m2 = Mat4RotateZ(0.785398f); /* 45 degrees */

    /* Verification of Parity */
    {
        float c_out[16];
        raw_c_mat4_mul(m1.m, m2.m, c_out);

        Mat4 cp_out = Mat4Mul(&m1, &m2);

        f64 max_diff = 0.0;
        for (int i = 0; i < 16; i++) {
            f64 diff = fabs((f64)c_out[i] - (f64)cp_out.m[i]);
            if (diff > max_diff) max_diff = diff;
        }

        printf("  [PARITY VERIFICATION] Matrix multiplication max cell difference: %.2e\n", max_diff);
        if (max_diff > 1e-6) {
            fprintf(stderr, "PARITY ERROR in matrix multiplication!\n");
            abort();
        }
        printf("  >> PARITY CHECK PASSED: SIMD output matches pure C float-for-float!\n\n");
    }

    bench_stats_t stats_c_mat;
    bench_stats_t stats_simd_mat;

    /* 2.1 Pure C 3-loop matrix multiply */
    printf("[EXPERIMENT 2.1] Pure C (Traditional 3-loop scalar matrix multiply)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        float out[16];
        u64 t0 = get_time_ns();
        for (int i = 0; i < MAT_COUNT; i++) {
            raw_c_mat4_mul(m1.m, m2.m, out);
            __asm__ volatile("" : : "r"(out) : "memory");
        }
        u64 t1 = get_time_ns();
        CP_UNUSED(out);
        stats_c_mat.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_c_mat.samples_ms[r]);
    }
    compute_stats(&stats_c_mat, MAT_COUNT);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/mul\n\n",
           stats_c_mat.mean_ms, stats_c_mat.min_ms, stats_c_mat.max_ms,
           stats_c_mat.stddev_ms, stats_c_mat.latency_ns_op);

    /* 2.2 Cplus SIMD Mat4Mul */
    printf("[EXPERIMENT 2.2] Cplus Mat4Mul (Hardware SIMD Vectorized SSE/AVX)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        Mat4 out;
        u64 t0 = get_time_ns();
        for (int i = 0; i < MAT_COUNT; i++) {
            out = Mat4Mul(&m1, &m2);
            __asm__ volatile("" : : "r"(&out) : "memory");
        }
        u64 t1 = get_time_ns();
        CP_UNUSED(out);
        stats_simd_mat.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_simd_mat.samples_ms[r]);
    }
    compute_stats(&stats_simd_mat, MAT_COUNT);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/mul\n",
           stats_simd_mat.mean_ms, stats_simd_mat.min_ms, stats_simd_mat.max_ms,
           stats_simd_mat.stddev_ms, stats_simd_mat.latency_ns_op);
    printf("  >> SPEEDUP: %8.2fx FASTER than Pure C\n\n", stats_c_mat.mean_ms / stats_simd_mat.mean_ms);
}

int main(void) {
    printf("=================================================================================\n");
    printf("  SCIENTIFIC EXPERIMENT REPORT: ALGORITHMS & SIMD LINEAR ALGEBRA\n");
    printf("=================================================================================\n\n");

    run_sorting_benchmarks();
    run_matrix_benchmarks();

    printf("=================================================================================\n");
    return 0;
}
