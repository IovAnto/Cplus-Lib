/**
 * @file bench_str_fs.c
 * @brief Scientific latency & throughput benchmark: Pure C vs Cplus StringView & File I/O.
 *        Executes multiple experimental runs to calculate statistical metrics:
 *        Min, Max, Mean, and Standard Deviation.
 */

#include "cplus/cplus.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_RUNS 5
#define SPLIT_ITERATIONS 500000
#define FILE_IO_ITERATIONS 2000

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

int main(void) {
    printf("=================================================================================\n");
    printf("  SCIENTIFIC EXPERIMENT REPORT: STRING PARSING & FILE I/O LATENCY\n");
    printf("=================================================================================\n\n");

    /* =========================================================================
     * EXPERIMENT 1: CSV Tokenization / Splitting (500,000 iterations)
     * ========================================================================= */
    const char *csv_sample = "timestamp=2026-11-12T05:00:00Z,sensor_id=TEMP_402,voltage=3.312,status=OPTIMAL";
    usize csv_len = strlen(csv_sample);
    printf("--- PART A: CSV Tokenization (Length: %zu bytes, 4 tokens x %d iterations) ---\n\n",
           csv_len, SPLIT_ITERATIONS);

    bench_stats_t stats_strtok;
    bench_stats_t stats_strview;

    /* Pure C: strdup + strtok_r */
    printf("[EXPERIMENT 1.1] Pure C (strdup + strtok_r tokenization + free copy)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        volatile usize token_count = 0;
        for (int i = 0; i < SPLIT_ITERATIONS; i++) {
            char *buf = strdup(csv_sample);
            char *saveptr = NULL;
            char *token = strtok_r(buf, ",", &saveptr);
            while (token) {
                token_count++;
                token = strtok_r(NULL, ",", &saveptr);
            }
            free(buf);
        }
        u64 t1 = get_time_ns();
        CP_UNUSED(token_count);
        stats_strtok.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_strtok.samples_ms[r]);
    }
    compute_stats(&stats_strtok, SPLIT_ITERATIONS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n\n",
           stats_strtok.mean_ms, stats_strtok.min_ms, stats_strtok.max_ms,
           stats_strtok.stddev_ms, stats_strtok.latency_ns_op);

    /* Cplus: StrSplit (Zero Allocation) */
    printf("[EXPERIMENT 1.2] Cplus StrSplit (Zero-Allocation StrView Slices)\n");
    StrView csv_sv = StrFromParts(csv_sample, csv_len);
    StrView tokens[8];

    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        volatile usize token_count = 0;
        for (int i = 0; i < SPLIT_ITERATIONS; i++) {
            usize count = StrSplit(csv_sv, ',', tokens, 8);
            token_count += count;
        }
        u64 t1 = get_time_ns();
        CP_UNUSED(token_count);
        stats_strview.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_strview.samples_ms[r]);
    }
    compute_stats(&stats_strview, SPLIT_ITERATIONS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n",
           stats_strview.mean_ms, stats_strview.min_ms, stats_strview.max_ms,
           stats_strview.stddev_ms, stats_strview.latency_ns_op);
    printf("  >> SPEEDUP: %8.2fx FASTER than Pure C\n\n", stats_strtok.mean_ms / stats_strview.mean_ms);

    /* =========================================================================
     * EXPERIMENT 2: Whole File Read (2,000 file I/O cycles)
     * ========================================================================= */
    const char *test_path = "/tmp/cplus_bench_io.txt";
    const char *payload = "PAYLOAD_BLOCK_0123456789_CPLUS_HIGH_SPEED_BUFFER_TEST\n"
                          "SECOND_ROW_DATA_FOR_FILE_BENCHMARKING_STREAM\n";
    usize payload_len = strlen(payload);
    FileWrite(test_path, payload, payload_len);

    printf("--- PART B: Whole File Read (Payload: %zu bytes x %d iterations) ---\n\n",
           payload_len, FILE_IO_ITERATIONS);

    bench_stats_t stats_c_io;
    bench_stats_t stats_cp_io;

    /* Pure C: fopen / fseek / ftell / malloc / fread / fclose / free */
    printf("[EXPERIMENT 2.1] Pure C (fopen + fseek + ftell + malloc + fread + fclose + free)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        volatile usize total_read = 0;
        for (int i = 0; i < FILE_IO_ITERATIONS; i++) {
            FILE *f = fopen(test_path, "rb");
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *buf = (char *)malloc((usize)sz + 1);
            usize rd = fread(buf, 1, (usize)sz, f);
            fclose(f);
            total_read += rd;
            free(buf);
        }
        u64 t1 = get_time_ns();
        CP_UNUSED(total_read);
        stats_c_io.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_c_io.samples_ms[r]);
    }
    compute_stats(&stats_c_io, FILE_IO_ITERATIONS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n\n",
           stats_c_io.mean_ms, stats_c_io.min_ms, stats_c_io.max_ms,
           stats_c_io.stddev_ms, stats_c_io.latency_ns_op);

    /* Cplus: FileRead with Arena Reset */
    printf("[EXPERIMENT 2.2] Cplus FileRead (One-Liner + Arena Batch Recycling)\n");
    cp_arena_t *io_arena = ArenaCreate(64 * 1024);
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        volatile usize total_read = 0;
        for (int i = 0; i < FILE_IO_ITERATIONS; i++) {
            StrView content = FileRead(io_arena, test_path);
            total_read += content.len;
            ArenaReset(io_arena);
        }
        u64 t1 = get_time_ns();
        CP_UNUSED(total_read);
        stats_cp_io.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_cp_io.samples_ms[r]);
    }
    ArenaDestroy(io_arena);
    compute_stats(&stats_cp_io, FILE_IO_ITERATIONS);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/op\n",
           stats_cp_io.mean_ms, stats_cp_io.min_ms, stats_cp_io.max_ms,
           stats_cp_io.stddev_ms, stats_cp_io.latency_ns_op);
    printf("  >> SPEEDUP: %8.2fx FASTER than Pure C\n\n", stats_c_io.mean_ms / stats_cp_io.mean_ms);

    FileRemove(test_path);
    printf("=================================================================================\n");
    return 0;
}
