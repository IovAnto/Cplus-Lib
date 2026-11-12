/**
 * @file cp_time.h
 * @brief High-precision monotonic timers, stopwatch, ISO8601 timestamping, and RAII scoped profiling.
 */

#ifndef CPLUS_TIME_H
#define CPLUS_TIME_H

#include "cp_core.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Monotonic Time Primitives
 * ------------------------------------------------------------------------- */
u64 cp_time_now_ns(void);
f64 cp_time_now_us(void);
f64 cp_time_now_ms(void);
u64 cp_time_unix_sec(void);

void cp_time_sleep_ms(u32 ms);
void cp_time_sleep_us(u32 us);

void cp_time_iso8601(char *buf, usize cap);

/* -------------------------------------------------------------------------
 * Stopwatch
 * ------------------------------------------------------------------------- */
typedef struct {
    u64 start_ns;
    u64 stop_ns;
    bool running;
} cp_stopwatch_t;

cp_stopwatch_t cp_stopwatch_start(void);
void cp_stopwatch_stop(cp_stopwatch_t *sw);
void cp_stopwatch_reset(cp_stopwatch_t *sw);
f64 cp_stopwatch_elapsed_ns(const cp_stopwatch_t *sw);
f64 cp_stopwatch_elapsed_us(const cp_stopwatch_t *sw);
f64 cp_stopwatch_elapsed_ms(const cp_stopwatch_t *sw);
f64 cp_stopwatch_elapsed_sec(const cp_stopwatch_t *sw);

/* -------------------------------------------------------------------------
 * Scoped RAII Profiler
 * ------------------------------------------------------------------------- */
typedef struct {
    const char *name;
    u64 start_ns;
} _cp_prof_scope_t;

static inline void _cp_prof_scope_cleanup(_cp_prof_scope_t *scope) {
    if (scope && scope->name) {
        u64 diff = cp_time_now_ns() - scope->start_ns;
        printf("[PROFILE] \"%s\": %.3f ms (%.1f us)\n", scope->name, (f64)diff / 1000000.0, (f64)diff / 1000.0);
    }
}

#define ProfileScope(name_str) \
    _cp_prof_scope_t CP_CONCAT(_prof_scope_, __LINE__) __attribute__((cleanup(_cp_prof_scope_cleanup), unused)) = \
        { .name = (name_str), .start_ns = cp_time_now_ns() }

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define TimeNowNs()                      cp_time_now_ns()
#define TimeNowUs()                      cp_time_now_us()
#define TimeNowMs()                      cp_time_now_ms()
#define TimeUnixSec()                    cp_time_unix_sec()
#define TimeSleepMs(ms)                  cp_time_sleep_ms(ms)
#define TimeSleepUs(us)                  cp_time_sleep_us(us)
#define TimeIso8601(buf, cap)            cp_time_iso8601(buf, cap)

#define StopwatchStart()                 cp_stopwatch_start()
#define StopwatchStop(sw)                cp_stopwatch_stop(sw)
#define StopwatchReset(sw)               cp_stopwatch_reset(sw)
#define StopwatchElapsedNs(sw)           cp_stopwatch_elapsed_ns(sw)
#define StopwatchElapsedUs(sw)           cp_stopwatch_elapsed_us(sw)
#define StopwatchElapsedMs(sw)           cp_stopwatch_elapsed_ms(sw)
#define StopwatchElapsedSec(sw)          cp_stopwatch_elapsed_sec(sw)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    u64 (*const nowNs)(void);
    f64 (*const nowUs)(void);
    f64 (*const nowMs)(void);
    u64 (*const unixSec)(void);
    void (*const sleepMs)(u32 ms);
    void (*const sleepUs)(u32 us);
    void (*const iso8601)(char *buf, usize cap);
} cp_time_namespace_t;

typedef struct {
    cp_stopwatch_t (*const start)(void);
    void (*const stop)(cp_stopwatch_t *sw);
    void (*const reset)(cp_stopwatch_t *sw);
    f64 (*const elapsedNs)(const cp_stopwatch_t *sw);
    f64 (*const elapsedUs)(const cp_stopwatch_t *sw);
    f64 (*const elapsedMs)(const cp_stopwatch_t *sw);
    f64 (*const elapsedSec)(const cp_stopwatch_t *sw);
} cp_stopwatch_namespace_t;

static const cp_time_namespace_t time_api = {
    .nowNs = cp_time_now_ns,
    .nowUs = cp_time_now_us,
    .nowMs = cp_time_now_ms,
    .unixSec = cp_time_unix_sec,
    .sleepMs = cp_time_sleep_ms,
    .sleepUs = cp_time_sleep_us,
    .iso8601 = cp_time_iso8601,
};

static const cp_stopwatch_namespace_t stopwatch = {
    .start = cp_stopwatch_start,
    .stop = cp_stopwatch_stop,
    .reset = cp_stopwatch_reset,
    .elapsedNs = cp_stopwatch_elapsed_ns,
    .elapsedUs = cp_stopwatch_elapsed_us,
    .elapsedMs = cp_stopwatch_elapsed_ms,
    .elapsedSec = cp_stopwatch_elapsed_sec,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_TIME_H */
