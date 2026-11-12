# Module Specification: cp_thread (Concurrency & Synchronization)

## 1. TL;DR
The `cp_thread` module provides pre-warmed worker thread pooling (`ThreadPool`), work-sharing loops (`ParallelFor`), scoped RAII mutex locks (`ScopedLock`, `WithLock`), condition variables (`CondVar`), and C11 atomic primitives without external runtime dependencies (zero OpenMP required).

```c
#include "cplus/cplus.h"

ThreadPool *pool = ThreadPoolCreate(0, 0); // Auto-detect CPU cores

/* 1. Asynchronous Task with Closure */
ThreadPoolSubmit(pool, Lambda(void, (void *arg), {
    printf("Task executing on background worker!\n");
}), NULL);

/* 2. Parallel-For Range Partitioning with Barrier Sync */
ParallelFor(pool, 1000000, Lambda(void, (usize start, usize end, void *arg), {
    float *arr = (float *)arg;
    for (usize i = start; i < end; i++) arr[i] *= 2.0f;
}), data);

ThreadPoolDestroy(pool);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Struct Layout: `ThreadPool` (`cp_threadpool_t`)
```c
typedef struct {
    void (*fn)(void *arg);
    void *arg;
} cp_pool_task_t;

struct cp_threadpool {
    cp_thread_t *threads;
    usize num_threads;
    cp_pool_task_t *queue;
    usize queue_capacity;
    usize queue_head;
    usize queue_tail;
    usize queue_count;
    usize active_workers;
    cp_mutex_t lock;
    cp_cond_t not_empty;
    cp_cond_t not_full;
    cp_cond_t all_idle;
    bool shutdown;
};
```
- **Pre-Warmed Workers**: Threads are instantiated once during `ThreadPoolCreate` and remain idle on `cp_cond_wait(&pool->not_empty, &pool->lock)` when no work is pending, eliminating `pthread_create` overhead.
- **Bounded Circular Queue**: Prevents unbounded memory growth. If the queue is full, `ThreadPoolSubmit` blocks on `not_full` until capacity opens up.
- **ParallelFor Calling Thread Participation**: When executing `ParallelFor`, the calling thread executes the final range slice directly on the current core while worker threads process the other slices in parallel, avoiding CPU core starvation and eliminating thread hopping.

### 2.2 RAII Scoped Locks
```c
#define ScopedLock(m_ptr) \
    pthread_mutex_t *CP_CONCAT(_scoped_mtx_, __LINE__) __attribute__((cleanup(_cp_scoped_mutex_unlock), unused)) = \
        (pthread_mutex_lock((pthread_mutex_t*)(m_ptr)), (pthread_mutex_t*)(m_ptr))

#define WithLock(m_ptr) \
    for (int _lock_done_ = (pthread_mutex_lock((pthread_mutex_t*)(m_ptr)), 0); \
         !_lock_done_; \
         pthread_mutex_unlock((pthread_mutex_t*)(m_ptr)), _lock_done_ = 1)
```
- Completely eliminates mutex leak deadlocks on unexpected early returns or goto labels.

---

## 3. Operational Guarantees & Contracts

| Metric | ThreadPool | Mutex / ScopedLock | Atomic Primitives |
|---|---|---|---|
| **Dispatch Latency** | $O(1)$ task queue insertion | $O(1)$ fast-path futex lock | $O(1)$ hardware instruction |
| **Worker Synchronization**| Condition variable signaling | OS futex / kernel sleep | Hardware cache coherence (`LOCK`) |
| **Memory Allocation** | Pre-allocated during init | 0 heap allocations | 0 heap allocations |
| **External Dependencies** | Standard `libpthread` | Standard `libpthread` | Inlined GCC/Clang builtins |
| **Thread-Safety** | Safe for concurrent submits | Thread-safe mutual exclusion | Lock-free sequential consistency |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Create Pool | `ThreadPoolCreate(threads, cap)` | `threadpool.create(...)` | Initialize pre-warmed pool (0 = auto cores) |
| Submit Task | `ThreadPoolSubmit(pool, fn, arg)` | `threadpool.submit(...)` | Blocking submit (blocks if queue is full) |
| Try Submit | `ThreadPoolTrySubmit(p, fn, arg)` | `threadpool.trySubmit(...)` | Non-blocking submit (returns false if full) |
| Parallel-For | `ParallelFor(pool, count, fn, arg)` | `threadpool.parallelFor(...)`| Parallel range processing with barrier sync |
| Parallel Vector | `VecParallelMap(pool, vec, body)` | N/A | In-place parallel dynamic array mapping |
| Wait Pool | `ThreadPoolWait(pool)` | `threadpool.wait(pool)` | Block until all queued/active tasks finish |
| Destroy Pool | `ThreadPoolDestroy(pool)` | `threadpool.destroy(pool)` | Join workers and free resources |
| Scoped Lock | `ScopedLock(&mutex)` | N/A | RAII block auto-unlock |
| With Lock | `WithLock(&mutex) { ... }` | N/A | Synchronized execution block |
| Atomic Add | `AtomicAdd(ptr, val)` | N/A | Inlined atomic increment/add |
| Atomic Store | `AtomicStore(ptr, val)` | N/A | Sequentially consistent atomic store |

---

## 5. Use Cases
1. **Parallel Raytracing & Image Filters**: Partition pixel scanlines across CPU cores using `ParallelFor`.
2. **Asynchronous I/O Dispatch**: Offload slow disk writes and database queries to the `ThreadPool`.
3. **Deadlock-Free State Protection**: Guard shared hash tables and caches using `ScopedLock(&m)`.

---

## 6. Complete C11 Usage Examples

### Example: Multi-Core Vector Computation
```c
#include "cplus/cplus.h"
#include <math.h>

void parallel_audio_normalize(ThreadPool *pool, float *samples, usize count) {
    // Process 1,000,000 audio samples partitioned evenly across all CPU cores
    ParallelFor(pool, count, Lambda(void, (usize start, usize end, void *arg), {
        float *buf = (float *)arg;
        for (usize i = start; i < end; i++) {
            buf[i] = tanhf(buf[i] * 1.5f);
        }
    }), samples);
}
```

---

## 7. Limitations & Operational Constraints
- **Recursive `ParallelFor`**: Do not invoke `ParallelFor` from inside a task that is itself already running on a thread pool worker, as this can cause worker starvation if all threads block waiting for sub-tasks.
- **Task Queue Saturation**: Default queue capacity is 4096. If you submit 100,000 independent tasks via `ThreadPoolSubmit` without waiting, the submitting thread will block until space opens up in the ring queue. Use `ParallelFor` instead of submitting single-item tasks.
