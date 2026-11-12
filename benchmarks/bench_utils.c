/**
 * @file bench_utils.c
 * @brief Statistical benchmarks for Xoshiro256** PRNG and RFC 4648 Base64.
 */

#define _POSIX_C_SOURCE 199309L
#include "cplus/cplus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define RUNS 5
#define BASE64_BYTES (10 * 1024 * 1024)
#define PRNG_OPS 20000000

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

static void bench_prng(void) {
    printf("\n=== BENCHMARK 1: PRNG Throughput (%d operations) ===\n", PRNG_OPS);
    double c_times[RUNS];
    double cp_times[RUNS];

    /* Baseline: libc rand() */
    for (int r = 0; r < RUNS; r++) {
        srand(42 + r);
        double t0 = get_time_sec();
        u64 acc = 0;
        for (int i = 0; i < PRNG_OPS; i++) {
            acc += (u64)rand();
        }
        __asm__ volatile("" : : "r"(acc) : "memory");
        double t1 = get_time_sec();
        c_times[r] = (t1 - t0) * 1000.0;
    }

    /* Cplus: Xoshiro256** */
    for (int r = 0; r < RUNS; r++) {
        cp_rand_t rng_state;
        RandInit(&rng_state, 42 + r);
        double t0 = get_time_sec();
        u64 acc = 0;
        for (int i = 0; i < PRNG_OPS; i++) {
            acc += RandU64(&rng_state);
        }
        __asm__ volatile("" : : "r"(acc) : "memory");
        double t1 = get_time_sec();
        cp_times[r] = (t1 - t0) * 1000.0;
    }

    stats_t sc = calc_stats(c_times, RUNS);
    stats_t scp = calc_stats(cp_times, RUNS);

    printf("Pure C libc rand():   Mean: %7.2f ms (Min: %7.2f, Max: %7.2f, StdDev: %5.2f ms) | %7.2f M ops/s\n",
           sc.mean, sc.min, sc.max, sc.stddev, (PRNG_OPS / 1e6) / (sc.mean / 1000.0));
    printf("Cplus Xoshiro256**:   Mean: %7.2f ms (Min: %7.2f, Max: %7.2f, StdDev: %5.2f ms) | %7.2f M ops/s\n",
           scp.mean, scp.min, scp.max, scp.stddev, (PRNG_OPS / 1e6) / (scp.mean / 1000.0));
    printf("Speedup: %.2fx faster than libc rand()\n", sc.mean / scp.mean);
}

static void bench_base64(void) {
    printf("\n=== BENCHMARK 2: Base64 Encoding (%d MB payload) ===\n", BASE64_BYTES / (1024 * 1024));
    u8 *input = (u8 *)malloc(BASE64_BYTES);
    for (usize i = 0; i < BASE64_BYTES; i++) {
        input[i] = (u8)(i * 37 + 13);
    }
    usize enc_cap = ((BASE64_BYTES + 2) / 3) * 4 + 1;
    char *c_out = (char *)malloc(enc_cap);
    char *cp_out = (char *)malloc(enc_cap);

    double c_times[RUNS];
    double cp_times[RUNS];

    static const char b64_tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (int r = 0; r < RUNS; r++) {
        double t0 = get_time_sec();
        usize j = 0;
        usize i = 0;
        for (; i + 2 < BASE64_BYTES; i += 3) {
            u32 tr = ((u32)input[i] << 16) | ((u32)input[i+1] << 8) | input[i+2];
            c_out[j++] = b64_tbl[(tr >> 18) & 0x3F];
            c_out[j++] = b64_tbl[(tr >> 12) & 0x3F];
            c_out[j++] = b64_tbl[(tr >> 6) & 0x3F];
            c_out[j++] = b64_tbl[tr & 0x3F];
        }
        if (i < BASE64_BYTES) {
            usize rem = BASE64_BYTES - i;
            if (rem == 1) {
                u32 tr = (u32)input[i] << 16;
                c_out[j++] = b64_tbl[(tr >> 18) & 0x3F];
                c_out[j++] = b64_tbl[(tr >> 12) & 0x3F];
                c_out[j++] = '=';
                c_out[j++] = '=';
            } else if (rem == 2) {
                u32 tr = ((u32)input[i] << 16) | ((u32)input[i+1] << 8);
                c_out[j++] = b64_tbl[(tr >> 18) & 0x3F];
                c_out[j++] = b64_tbl[(tr >> 12) & 0x3F];
                c_out[j++] = b64_tbl[(tr >> 6) & 0x3F];
                c_out[j++] = '=';
            }
        }
        c_out[j] = '\0';
        __asm__ volatile("" : : "r"(c_out) : "memory");
        double t1 = get_time_sec();
        c_times[r] = (t1 - t0) * 1000.0;
    }

    for (int r = 0; r < RUNS; r++) {
        double t0 = get_time_sec();
        i64 n = Base64Encode(input, BASE64_BYTES, cp_out, enc_cap);
        CP_UNUSED(n);
        __asm__ volatile("" : : "r"(cp_out) : "memory");
        double t1 = get_time_sec();
        cp_times[r] = (t1 - t0) * 1000.0;
    }

    if (memcmp(c_out, cp_out, enc_cap - 1) != 0) {
        fprintf(stderr, "FATAL: Base64 parity mismatch!\n");
        abort();
    }

    stats_t sc = calc_stats(c_times, RUNS);
    stats_t scp = calc_stats(cp_times, RUNS);

    double mb = (double)BASE64_BYTES / (1024.0 * 1024.0);
    printf("Naive Pure C Base64:  Mean: %7.2f ms (Min: %7.2f, Max: %7.2f, StdDev: %5.2f ms) | %7.2f MB/s\n",
           sc.mean, sc.min, sc.max, sc.stddev, mb / (sc.mean / 1000.0));
    printf("Cplus Base64Encode:   Mean: %7.2f ms (Min: %7.2f, Max: %7.2f, StdDev: %5.2f ms) | %7.2f MB/s\n",
           scp.mean, scp.min, scp.max, scp.stddev, mb / (scp.mean / 1000.0));
    printf("Speedup: %.2fx (Parity: 100%% bit-exact match on %zu bytes)\n", sc.mean / scp.mean, enc_cap - 1);

    free(input);
    free(c_out);
    free(cp_out);
}

int main(void) {
    printf("===================================================================\n");
    printf("  Cplus System Utilities Benchmark Suite\n");
    printf("===================================================================\n");
    bench_prng();
    bench_base64();
    printf("\nAll benchmarks completed successfully with bit-exact parity.\n");
    return 0;
}
