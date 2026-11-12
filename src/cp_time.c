/**
 * @file cp_time.c
 * @brief Implementation of monotonic time primitives, stopwatch, and ISO8601 formatting.
 */

#include "cplus/cp_time.h"
#include <stdio.h>
#include <string.h>

u64 cp_time_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

f64 cp_time_now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec * 1000000.0 + (f64)ts.tv_nsec / 1000.0;
}

f64 cp_time_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
}

u64 cp_time_unix_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (u64)ts.tv_sec;
}

void cp_time_sleep_ms(u32 ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

void cp_time_sleep_us(u32 us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (long)(us % 1000000) * 1000L;
    nanosleep(&ts, NULL);
}

void cp_time_iso8601(char *buf, usize cap) {
    if (!buf || cap < 21) return;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_buf;
    gmtime_r(&ts.tv_sec, &tm_buf);
    strftime(buf, cap, "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
}

/* -------------------------------------------------------------------------
 * Stopwatch
 * ------------------------------------------------------------------------- */
cp_stopwatch_t cp_stopwatch_start(void) {
    cp_stopwatch_t sw;
    sw.start_ns = cp_time_now_ns();
    sw.stop_ns = 0;
    sw.running = true;
    return sw;
}

void cp_stopwatch_stop(cp_stopwatch_t *sw) {
    if (sw && sw->running) {
        sw->stop_ns = cp_time_now_ns();
        sw->running = false;
    }
}

void cp_stopwatch_reset(cp_stopwatch_t *sw) {
    if (sw) {
        sw->start_ns = cp_time_now_ns();
        sw->stop_ns = 0;
        sw->running = true;
    }
}

f64 cp_stopwatch_elapsed_ns(const cp_stopwatch_t *sw) {
    if (!sw) return 0.0;
    u64 end = sw->running ? cp_time_now_ns() : sw->stop_ns;
    return (f64)(end - sw->start_ns);
}

f64 cp_stopwatch_elapsed_us(const cp_stopwatch_t *sw) {
    return cp_stopwatch_elapsed_ns(sw) / 1000.0;
}

f64 cp_stopwatch_elapsed_ms(const cp_stopwatch_t *sw) {
    return cp_stopwatch_elapsed_ns(sw) / 1000000.0;
}

f64 cp_stopwatch_elapsed_sec(const cp_stopwatch_t *sw) {
    return cp_stopwatch_elapsed_ns(sw) / 1000000000.0;
}
