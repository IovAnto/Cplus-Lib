/**
 * @file cp_thread.h
 * @brief Zero-overhead threading primitives, scoped RAII locks, and lock-free/low-latency thread pool.
 */

#ifndef CPLUS_THREAD_H
#define CPLUS_THREAD_H

#include "cp_core.h"
#include <pthread.h>
#include <sched.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Thread Primitives
 * ------------------------------------------------------------------------- */
typedef pthread_t cp_thread_t;
typedef void *(*cp_thread_fn)(void *);

CP_INLINE bool cp_thread_create(cp_thread_t *t, cp_thread_fn fn, void *arg) {
    return pthread_create(t, NULL, fn, arg) == 0;
}

CP_INLINE bool cp_thread_join(cp_thread_t t, void **retval) {
    return pthread_join(t, retval) == 0;
}

CP_INLINE bool cp_thread_detach(cp_thread_t t) {
    return pthread_detach(t) == 0;
}

CP_INLINE cp_thread_t cp_thread_self(void) {
    return pthread_self();
}

CP_INLINE void cp_thread_yield(void) {
    sched_yield();
}

/* -------------------------------------------------------------------------
 * Mutex & Scoped RAII Locks
 * ------------------------------------------------------------------------- */
typedef pthread_mutex_t cp_mutex_t;
typedef cp_mutex_t Mutex;

CP_INLINE void cp_mutex_init(cp_mutex_t *m) {
    pthread_mutex_init(m, NULL);
}

CP_INLINE void cp_mutex_destroy(cp_mutex_t *m) {
    pthread_mutex_destroy(m);
}

CP_INLINE void cp_mutex_lock(cp_mutex_t *m) {
    pthread_mutex_lock(m);
}

CP_INLINE void cp_mutex_unlock(cp_mutex_t *m) {
    pthread_mutex_unlock(m);
}

CP_INLINE bool cp_mutex_trylock(cp_mutex_t *m) {
    return pthread_mutex_trylock(m) == 0;
}

static inline void _cp_scoped_mutex_unlock(pthread_mutex_t **m) {
    if (m && *m) {
        pthread_mutex_unlock(*m);
    }
}

/**
 * @brief Automatic RAII Scoped Lock for any lexical block.
 * Releases the mutex automatically when execution leaves the enclosing scope.
 */
#define ScopedLock(m_ptr) \
    pthread_mutex_t *CP_CONCAT(_scoped_mtx_, __LINE__) __attribute__((cleanup(_cp_scoped_mutex_unlock), unused)) = \
        (pthread_mutex_lock((pthread_mutex_t*)(m_ptr)), (pthread_mutex_t*)(m_ptr))

/**
 * @brief Block-scoped RAII lock: WithLock(&mutex) { ... }
 */
#define WithLock(m_ptr) \
    for (int _lock_done_ = (pthread_mutex_lock((pthread_mutex_t*)(m_ptr)), 0); \
         !_lock_done_; \
         pthread_mutex_unlock((pthread_mutex_t*)(m_ptr)), _lock_done_ = 1)

/* -------------------------------------------------------------------------
 * Condition Variables
 * ------------------------------------------------------------------------- */
typedef pthread_cond_t cp_cond_t;
typedef cp_cond_t CondVar;

CP_INLINE void cp_cond_init(cp_cond_t *c) {
    pthread_cond_init(c, NULL);
}

CP_INLINE void cp_cond_destroy(cp_cond_t *c) {
    pthread_cond_destroy(c);
}

CP_INLINE void cp_cond_wait(cp_cond_t *c, cp_mutex_t *m) {
    pthread_cond_wait(c, m);
}

CP_INLINE void cp_cond_signal(cp_cond_t *c) {
    pthread_cond_signal(c);
}

CP_INLINE void cp_cond_broadcast(cp_cond_t *c) {
    pthread_cond_broadcast(c);
}

bool cp_cond_timedwait_ms(cp_cond_t *c, cp_mutex_t *m, u32 timeout_ms);

/* -------------------------------------------------------------------------
 * Atomic Primitives (C11 / GCC builtins)
 * ------------------------------------------------------------------------- */
#define AtomicInc(ptr)        __atomic_add_fetch((ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicDec(ptr)        __atomic_sub_fetch((ptr), 1, __ATOMIC_SEQ_CST)
#define AtomicAdd(ptr, val)   __atomic_add_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define AtomicSub(ptr, val)   __atomic_sub_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define AtomicLoad(ptr)       __atomic_load_n((ptr), __ATOMIC_SEQ_CST)
#define AtomicStore(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_SEQ_CST)

/* -------------------------------------------------------------------------
 * Low-Latency Thread Pool
 * ------------------------------------------------------------------------- */
typedef struct cp_threadpool cp_threadpool_t;
typedef cp_threadpool_t ThreadPool;

/**
 * @brief Create a pre-warmed worker thread pool.
 * @param num_threads Number of worker threads (0 = auto-detect hardware concurrency).
 * @param queue_capacity Maximum queued tasks before submit blocks (0 = default 4096).
 */
cp_threadpool_t *cp_threadpool_create(usize num_threads, usize queue_capacity);

/**
 * @brief Submit a task to the pool. Blocks if queue is full until space is available.
 */
bool cp_threadpool_submit(cp_threadpool_t *pool, void (*fn)(void *), void *arg);

/**
 * @brief Non-blocking attempt to submit a task. Returns false if queue is full.
 */
bool cp_threadpool_try_submit(cp_threadpool_t *pool, void (*fn)(void *), void *arg);

/**
 * @brief Block calling thread until all queued and running tasks have finished.
 */
void cp_threadpool_wait(cp_threadpool_t *pool);

/**
 * @brief Stop worker threads, drain pending queue, and free resources.
 */
void cp_threadpool_destroy(cp_threadpool_t *pool);

/**
 * @brief Get the number of worker threads in the pool.
 */
usize cp_threadpool_num_threads(const cp_threadpool_t *pool);

typedef void (*cp_parallel_for_fn)(usize start, usize end, void *arg);

/**
 * @brief Partition [0, count) across thread pool workers and synchronize on completion.
 * Calling thread participates in execution to maximize CPU utilization.
 */
void cp_threadpool_parallel_for(cp_threadpool_t *pool, usize count, cp_parallel_for_fn fn, void *arg);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define ThreadCreate(t, fn, arg)         cp_thread_create(t, fn, arg)
#define ThreadJoin(t, ret)               cp_thread_join(t, ret)
#define ThreadDetach(t)                  cp_thread_detach(t)
#define ThreadSelf()                     cp_thread_self()
#define ThreadYield()                    cp_thread_yield()

#define MutexInit(m)                     cp_mutex_init(m)
#define MutexDestroy(m)                  cp_mutex_destroy(m)
#define MutexLock(m)                     cp_mutex_lock(m)
#define MutexUnlock(m)                   cp_mutex_unlock(m)
#define MutexTryLock(m)                  cp_mutex_trylock(m)

#define CondInit(c)                      cp_cond_init(c)
#define CondDestroy(c)                   cp_cond_destroy(c)
#define CondWait(c, m)                   cp_cond_wait(c, m)
#define CondSignal(c)                    cp_cond_signal(c)
#define CondBroadcast(c)                 cp_cond_broadcast(c)
#define CondTimedWaitMs(c, m, ms)        cp_cond_timedwait_ms(c, m, ms)

#define ThreadPoolCreate(threads, cap)      cp_threadpool_create(threads, cap)
#define ThreadPoolSubmit(pool, fn, arg)     cp_threadpool_submit(pool, fn, arg)
#define ThreadPoolTrySubmit(p, fn, arg)     cp_threadpool_try_submit(p, fn, arg)
#define ThreadPoolWait(pool)                cp_threadpool_wait(pool)
#define ThreadPoolDestroy(pool)             cp_threadpool_destroy(pool)
#define ThreadPoolNumThreads(pool)          cp_threadpool_num_threads(pool)
#define ThreadPoolParallelFor(p, c, fn, a)  cp_threadpool_parallel_for(p, c, fn, a)
#define ParallelFor(pool, count, fn, arg)   cp_threadpool_parallel_for(pool, count, fn, arg)

#if defined(CP_HAS_LAMBDA) && CP_HAS_LAMBDA
/**
 * @brief In-place parallel vector mapping using thread pool and inlined closure.
 */
#define VecParallelMap(pool, v, body) do { \
    usize _vp_len = VecLen(v); \
    if ((pool) && _vp_len > 0) { \
        cp_threadpool_parallel_for((pool), _vp_len, Lambda(void, (usize _p_start, usize _p_end, void *_p_vec_raw), { \
            __typeof__(v) _p_arr = (__typeof__(v))_p_vec_raw; \
            for (usize _p_i = _p_start; _p_i < _p_end; _p_i++) { \
                __typeof__(v) it = &_p_arr[_p_i]; \
                CP_UNUSED(it); \
                body; \
            } \
        }), (v)); \
    } \
} while(0)
#endif

/* -------------------------------------------------------------------------
 * Dot-Notation Namespaces
 * ------------------------------------------------------------------------- */
typedef struct {
    bool (*const create)(cp_thread_t *t, cp_thread_fn fn, void *arg);
    bool (*const join)(cp_thread_t t, void **retval);
    bool (*const detach)(cp_thread_t t);
    cp_thread_t (*const self)(void);
    void (*const yield)(void);
} cp_thread_namespace_t;

typedef struct {
    void (*const init)(cp_mutex_t *m);
    void (*const destroy)(cp_mutex_t *m);
    void (*const lock)(cp_mutex_t *m);
    void (*const unlock)(cp_mutex_t *m);
    bool (*const trylock)(cp_mutex_t *m);
} cp_mutex_namespace_t;

typedef struct {
    cp_threadpool_t *(*const create)(usize num_threads, usize queue_capacity);
    bool (*const submit)(cp_threadpool_t *pool, void (*fn)(void *), void *arg);
    bool (*const trySubmit)(cp_threadpool_t *pool, void (*fn)(void *), void *arg);
    void (*const parallelFor)(cp_threadpool_t *pool, usize count, cp_parallel_for_fn fn, void *arg);
    usize (*const numThreads)(const cp_threadpool_t *pool);
    void (*const wait)(cp_threadpool_t *pool);
    void (*const destroy)(cp_threadpool_t *pool);
} cp_threadpool_namespace_t;

static const cp_thread_namespace_t thread = {
    .create = cp_thread_create,
    .join = cp_thread_join,
    .detach = cp_thread_detach,
    .self = cp_thread_self,
    .yield = cp_thread_yield,
};

static const cp_mutex_namespace_t mutex = {
    .init = cp_mutex_init,
    .destroy = cp_mutex_destroy,
    .lock = cp_mutex_lock,
    .unlock = cp_mutex_unlock,
    .trylock = cp_mutex_trylock,
};

static const cp_threadpool_namespace_t threadpool = {
    .create = cp_threadpool_create,
    .submit = cp_threadpool_submit,
    .trySubmit = cp_threadpool_try_submit,
    .parallelFor = cp_threadpool_parallel_for,
    .numThreads = cp_threadpool_num_threads,
    .wait = cp_threadpool_wait,
    .destroy = cp_threadpool_destroy,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_THREAD_H */
