/**
 * @file quickstart.c
 * @brief Concise introduction to C+ core ergonomics in ~40 lines of code.
 */

#include "cplus/cplus.h"

int main(void) {
    Println("=== C+ Quickstart Demonstration ===");

    /* 1. Linear Memory Arena */
    Arena *arena = ArenaCreate(64 * 1024);
    assert(arena != NULL);

    /* 2. Fast Bump Allocation */
    int *sensor_readings = (int *)ArenaAlloc(arena, 4 * sizeof(int));
    for (int i = 0; i < 4; i++) {
        sensor_readings[i] = (i + 1) * 25;
    }

    /* 3. Type-Safe Stretchy Vector */
    Vec(float) temperatures = VecNew(float);
    VecPush(temperatures, 21.5f);
    VecPush(temperatures, 22.8f);
    VecPush(temperatures, 24.1f);

    Print("Recorded Temperatures (C): ");
    VecEach(temperatures, idx, val, {
        printf("[#%zu: %.1f] ", idx, (double)val);
    });
    putchar('\n');

    /* 4. Open-Addressing Hash Map */
    HashMap *cache = MapCreate(16, arena);
    MapSet(cache, "srv-alpha", (void *)(uintptr_t)8080);
    MapSet(cache, "srv-beta",  (void *)(uintptr_t)8443);

    uintptr_t port = (uintptr_t)MapGet(cache, "srv-alpha");
    Println("Server Alpha Port:");
    Println((u64)port);

    /* 5. Monotonic Nanosecond Stopwatch */
    cp_stopwatch_t sw = StopwatchStart();
    TimeSleepMs(2);
    StopwatchStop(&sw);
    printf("Measured latency: %.2f ms\n", StopwatchElapsedMs(&sw));

    /* Clean up allocated dynamic resources */
    VecFree(temperatures);
    ArenaDestroy(arena);

    Println("Quickstart completed successfully.");
    return 0;
}
