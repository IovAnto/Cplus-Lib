# Module Specification: cp_time (Time, Stopwatch & Profiling Subsystem)

## 1. TL;DR
The `cp_time` subsystem delivers nanosecond-precision monotonic timing, wall-clock ISO 8601 timestamps, explicit stopwatch measurements, and RAII-based block profiling without external library dependencies.

```c
#include "cplus/cplus.h"

/* 1. Nanosecond Monotonic Timestamp */
u64 t0 = TimeNowNs();

/* 2. RAII Auto-Cleaning Block Profiler */
{
    ProfileScope("Matrix Multiplication");
    // Code executes here; elapsed time is printed automatically on scope exit
}

/* 3. Explicit Stopwatch */
cp_stopwatch_t sw = StopwatchStart();
// ... work ...
StopwatchStop(&sw);
printf("Elapsed: %.2f ms\n", StopwatchElapsedMs(&sw));

/* 4. ISO 8601 String */
char ts[32];
TimeIso8601(ts, sizeof(ts));
printf("UTC Timestamp: %s\n", ts);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Monotonic Clock Engine
- On POSIX platforms, `TimeNowNs` calls `clock_gettime(CLOCK_MONOTONIC, &ts)`.
- It converts seconds and nanoseconds into a continuous unsigned 64-bit nanosecond integer: `(u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec`.
- Guaranteed to never step backward, even if the system wall-clock adjusts due to NTP synchronization or daylight saving shifts.

### 2.2 Struct Layout: `cp_stopwatch_t`
```c
typedef struct {
    u64 start_ns;
    u64 stop_ns;
    bool running;
} cp_stopwatch_t;
```
- Can be started, stopped, reset, or queried across various units (nanoseconds, microseconds, milliseconds, seconds).

### 2.3 RAII Profiler: `ProfileScope`
```c
typedef struct {
    const char *name;
    u64 start_ns;
} _cp_prof_scope_t;

#define ProfileScope(name_str) \
    _cp_prof_scope_t CP_CONCAT(_prof_scope_, __LINE__) __attribute__((cleanup(_cp_prof_scope_cleanup), unused)) = \
        { .name = (name_str), .start_ns = cp_time_now_ns() }
```
- Uses GCC/Clang `__attribute__((cleanup))` to invoke `_cp_prof_scope_cleanup` automatically when leaving the lexical block.
- Zero runtime overhead when not executing; 0 heap allocations.

---

## 3. Operational Guarantees & Contracts

| Function | Precision | Clock Source | Monotonic | Heap Allocations |
|---|---|---|---|---|
| `TimeNowNs` | 1 ns | `CLOCK_MONOTONIC` | Yes | 0 |
| `TimeNowUs` | 1 us | `CLOCK_MONOTONIC` | Yes | 0 |
| `TimeNowMs` | 1 ms | `CLOCK_MONOTONIC` | Yes | 0 |
| `TimeUnixSec` | 1 s | `CLOCK_REALTIME` | No (Wall clock) | 0 |
| `TimeIso8601` | 1 s | `gmtime_r` / `gmtime` | No (Wall clock) | 0 |
| `ProfileScope` | 1 ns | `CLOCK_MONOTONIC` | Yes | 0 |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Now (Nanoseconds) | `TimeNowNs()` | `time_api.nowNs()` | Current monotonic timestamp in nanoseconds |
| Now (Microseconds) | `TimeNowUs()` | `time_api.nowUs()` | Current monotonic timestamp in fractional microseconds |
| Now (Milliseconds) | `TimeNowMs()` | `time_api.nowMs()` | Current monotonic timestamp in fractional milliseconds |
| Unix Epoch Seconds | `TimeUnixSec()` | `time_api.unixSec()` | Standard UTC timestamp in seconds since 1970 |
| Sleep (ms) | `TimeSleepMs(ms)` | `time_api.sleepMs(ms)` | Suspend thread execution for specified milliseconds |
| Sleep (us) | `TimeSleepUs(us)` | `time_api.sleepUs(us)` | Suspend thread execution for specified microseconds |
| ISO 8601 Stamp | `TimeIso8601(buf, cap)`| `time_api.iso8601(buf, cap)`| Format UTC timestamp as `YYYY-MM-DDTHH:MM:SSZ` |
| Start Stopwatch | `StopwatchStart()` | `stopwatch.start()` | Instantiate and activate a new stopwatch |
| Stop Stopwatch | `StopwatchStop(sw)` | `stopwatch.stop(sw)` | Pause active stopwatch |
| Reset Stopwatch | `StopwatchReset(sw)` | `stopwatch.reset(sw)` | Reset stopwatch counters to zero |
| Elapsed Ns | `StopwatchElapsedNs(sw)`| `stopwatch.elapsedNs(sw)`| Calculate elapsed nanoseconds |
| Elapsed Us | `StopwatchElapsedUs(sw)`| `stopwatch.elapsedUs(sw)`| Calculate elapsed microseconds |
| Elapsed Ms | `StopwatchElapsedMs(sw)`| `stopwatch.elapsedMs(sw)`| Calculate elapsed milliseconds |
| Elapsed Sec | `StopwatchElapsedSec(sw)`| `stopwatch.elapsedSec(sw)`| Calculate elapsed seconds |
| Profile Scope | `ProfileScope(name)` | - | RAII block benchmark logger |

---

## 5. Use Cases
1. **Low-Latency Benchmarking**: Measure subroutine latency down to single-digit nanoseconds without profiler skew.
2. **Deterministic Frame Pacing**: Maintain fixed 60 FPS / 144 FPS game loop ticks by computing `delta_time = TimeNowNs() - last_frame_ns`.
3. **Structured Event Logs**: Prefix telemetry and network packets with standardized ISO 8601 strings.
4. **Scope Profiling**: Rapidly identify algorithmic bottlenecks across nested blocks using `ProfileScope`.

---

## 6. Complete Compilable Example

```c
#include "cplus/cplus.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    /* 1. Monotonic timing */
    u64 t1 = time_api.nowNs();
    time_api.sleepMs(5);
    u64 t2 = time_api.nowNs();
    assert(t2 > t1);
    printf("Sleep interval: %llu ns\n", (unsigned long long)(t2 - t1));

    /* 2. Stopwatch */
    cp_stopwatch_t sw = stopwatch.start();
    time_api.sleepMs(10);
    stopwatch.stop(&sw);

    f64 ms = stopwatch.elapsedMs(&sw);
    assert(ms >= 9.0);
    printf("Stopwatch measured: %.2f ms\n", ms);

    /* 3. ISO 8601 Formatting */
    char buf[32];
    time_api.iso8601(buf, sizeof(buf));
    assert(strlen(buf) == 20); // "YYYY-MM-DDTHH:MM:SSZ"
    printf("Current ISO timestamp: %s\n", buf);

    /* 4. RAII Scoped profiler */
    {
        ProfileScope("Transient Compute Block");
        volatile int sum = 0;
        for (int i = 0; i < 100000; i++) {
            sum += i;
        }
    }

    return 0;
}
```

---

## 7. Limitations & Edge Cases
- **Resolution Dependent on Kernel**: While `TimeNowNs` returns integer nanoseconds, hardware timers and OS kernel ticks dictate the ultimate precision (typically 1 to 10 ns on modern x86-64 hardware).
- **Sleep Granularity**: `TimeSleepMs` and `TimeSleepUs` call the OS scheduler. Actual sleep duration may be extended by scheduler scheduling quanta (~0.1 ms to 1 ms).
