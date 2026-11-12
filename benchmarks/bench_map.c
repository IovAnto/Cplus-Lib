/**
 * @file bench_map.c
 * @brief Scientific latency & throughput benchmark: Hash Map lookups (1,000,000 ops)
 *        and SHA-256 cryptographic throughput (10,000,000 bytes).
 */

#include "cplus/cplus.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define NUM_RUNS 5
#define MAP_KEYS 100000
#define LOOKUP_OPS 1000000
#define SHA_BYTES (10 * 1024 * 1024)

typedef struct {
    f64 samples_ms[NUM_RUNS];
    f64 min_ms;
    f64 max_ms;
    f64 mean_ms;
    f64 stddev_ms;
    f64 latency_ns_op;
    f64 throughput_ops_sec;
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
    stats->throughput_ops_sec = ((f64)total_ops / stats->mean_ms) * 1000.0;
}

static void bench_hashmap(void) {
    printf("===============================================================================\n");
    printf(" EXPERIMENT 1: Open-Addressing Hash Map (100,000 Keys, 1,000,000 Lookups)      \n");
    printf("===============================================================================\n");

    cp_map_t *m = MapCreate(MAP_KEYS * 2, NULL);
    char **keys = (char **)malloc(MAP_KEYS * sizeof(char *));

    for (usize i = 0; i < MAP_KEYS; i++) {
        keys[i] = (char *)malloc(24);
        snprintf(keys[i], 24, "key_%07zu", i);
        MapSet(m, keys[i], (void *)(intptr_t)(i + 1));
    }

    bench_stats_t lookup_stats;
    u64 accumulator = 0;

    for (int run = 0; run < NUM_RUNS; run++) {
        u64 t0 = get_time_ns();
        u64 acc = 0;

        for (usize i = 0; i < LOOKUP_OPS; i++) {
            usize k_idx = (i * 37) % MAP_KEYS;
            void *val = MapGet(m, keys[k_idx]);
            acc += (u64)(intptr_t)val;
        }
        __asm__ volatile("" : : "r"(acc) : "memory");
        u64 t1 = get_time_ns();
        lookup_stats.samples_ms[run] = (f64)(t1 - t0) / 1000000.0;
        accumulator = acc;
    }
    compute_stats(&lookup_stats, LOOKUP_OPS);

    assert(accumulator > 0);
    CP_UNUSED(accumulator);

    printf("Method              Mean (ms)  Min (ms)   Max (ms)   StdDev (ms)  Latency/op     Throughput\n");
    printf("-----------------------------------------------------------------------------------------\n");
    printf("cp_map Lookup       %8.2f   %8.2f   %8.2f   %9.3f    %8.1f ns   %10.0f lookups/s\n",
           lookup_stats.mean_ms, lookup_stats.min_ms, lookup_stats.max_ms, lookup_stats.stddev_ms,
           lookup_stats.latency_ns_op, lookup_stats.throughput_ops_sec);
    printf("-----------------------------------------------------------------------------------------\n");
    printf("Parity Check: Non-trivial accumulator = 0x%016llX verified.\n\n", (unsigned long long)accumulator);

    for (usize i = 0; i < MAP_KEYS; i++) {
        free(keys[i]);
    }
    free(keys);
    MapDestroy(m);
}

static void bench_sha256(void) {
    printf("===============================================================================\n");
    printf(" EXPERIMENT 2: Cryptographic SHA-256 Bandwidth (10,000,000 Bytes)              \n");
    printf("===============================================================================\n");

    u8 *data = (u8 *)malloc(SHA_BYTES);
    for (usize i = 0; i < SHA_BYTES; i++) {
        data[i] = (u8)(i * 31 + 7);
    }

    bench_stats_t sha_stats;
    u8 hash[32];
    char hex[65];

    for (int run = 0; run < NUM_RUNS; run++) {
        u64 t0 = get_time_ns();
        Sha256(data, SHA_BYTES, hash);
        u64 t1 = get_time_ns();
        sha_stats.samples_ms[run] = (f64)(t1 - t0) / 1000000.0;
    }
    compute_stats(&sha_stats, SHA_BYTES);

    Sha256Hex(hash, hex);
    f64 mb_per_sec = ((f64)SHA_BYTES / (1024.0 * 1024.0)) / (sha_stats.mean_ms / 1000.0);

    printf("Method              Mean (ms)  Min (ms)   Max (ms)   StdDev (ms)  Bandwidth (MB/s)\n");
    printf("-----------------------------------------------------------------------------------------\n");
    printf("Cplus SHA-256       %8.2f   %8.2f   %8.2f   %9.3f    %8.1f MB/s\n",
           sha_stats.mean_ms, sha_stats.min_ms, sha_stats.max_ms, sha_stats.stddev_ms, mb_per_sec);
    printf("-----------------------------------------------------------------------------------------\n");
    printf("SHA-256 Output Hash: %s\n\n", hex);

    free(data);
}

int main(void) {
    printf("###############################################################################\n");
    printf("#         CPLUS BENCHMARK SUITE: HASH MAP & CRYPTOGRAPHIC HASHING             #\n");
    printf("#                   N=5 Statistical Samples with Parity Verification          #\n");
    printf("###############################################################################\n\n");

    bench_hashmap();
    bench_sha256();

    return 0;
}
