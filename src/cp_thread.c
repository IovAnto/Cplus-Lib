/**
 * @file cp_thread.c
 * @brief Implementation of low-latency worker thread pool and threading utilities.
 */

#include "cplus/cp_thread.h"
#include <unistd.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Condition Variable Timed Wait
 * ------------------------------------------------------------------------- */
bool cp_cond_timedwait_ms(cp_cond_t *c, cp_mutex_t *m, u32 timeout_ms) {
    if (!c || !m) return false;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return pthread_cond_timedwait(c, m, &ts) == 0;
}

/* -------------------------------------------------------------------------
 * Low-Latency Thread Pool Implementation
 * ------------------------------------------------------------------------- */
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

static void *cp_threadpool_worker(void *arg) {
    cp_threadpool_t *pool = (cp_threadpool_t *)arg;
    while (1) {
        cp_mutex_lock(&pool->lock);
        while (pool->queue_count == 0 && !pool->shutdown) {
            cp_cond_wait(&pool->not_empty, &pool->lock);
        }
        if (pool->shutdown && pool->queue_count == 0) {
            cp_mutex_unlock(&pool->lock);
            break;
        }

        cp_pool_task_t task = pool->queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % pool->queue_capacity;
        pool->queue_count--;
        pool->active_workers++;
        cp_cond_signal(&pool->not_full);
        cp_mutex_unlock(&pool->lock);

        if (task.fn) {
            task.fn(task.arg);
        }

        cp_mutex_lock(&pool->lock);
        pool->active_workers--;
        if (pool->queue_count == 0 && pool->active_workers == 0) {
            cp_cond_broadcast(&pool->all_idle);
        }
        cp_mutex_unlock(&pool->lock);
    }
    return NULL;
}

cp_threadpool_t *cp_threadpool_create(usize num_threads, usize queue_capacity) {
    if (num_threads == 0) {
        long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
        num_threads = (nprocs > 0) ? (usize)nprocs : 4;
    }
    if (queue_capacity == 0) {
        queue_capacity = 4096;
    }

    cp_threadpool_t *pool = (cp_threadpool_t *)calloc(1, sizeof(cp_threadpool_t));
    if (!pool) return NULL;

    pool->num_threads = num_threads;
    pool->queue_capacity = queue_capacity;
    pool->queue = (cp_pool_task_t *)calloc(queue_capacity, sizeof(cp_pool_task_t));
    pool->threads = (cp_thread_t *)calloc(num_threads, sizeof(cp_thread_t));
    if (!pool->queue || !pool->threads) {
        free(pool->queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    cp_mutex_init(&pool->lock);
    cp_cond_init(&pool->not_empty);
    cp_cond_init(&pool->not_full);
    cp_cond_init(&pool->all_idle);

    for (usize i = 0; i < num_threads; ++i) {
        if (pthread_create(&pool->threads[i], NULL, cp_threadpool_worker, pool) != 0) {
            pool->shutdown = true;
            cp_threadpool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

bool cp_threadpool_submit(cp_threadpool_t *pool, void (*fn)(void *), void *arg) {
    if (!pool || !fn) return false;
    cp_mutex_lock(&pool->lock);
    while (pool->queue_count == pool->queue_capacity && !pool->shutdown) {
        cp_cond_wait(&pool->not_full, &pool->lock);
    }
    if (pool->shutdown) {
        cp_mutex_unlock(&pool->lock);
        return false;
    }

    pool->queue[pool->queue_tail].fn = fn;
    pool->queue[pool->queue_tail].arg = arg;
    pool->queue_tail = (pool->queue_tail + 1) % pool->queue_capacity;
    pool->queue_count++;
    cp_cond_signal(&pool->not_empty);
    cp_mutex_unlock(&pool->lock);
    return true;
}

bool cp_threadpool_try_submit(cp_threadpool_t *pool, void (*fn)(void *), void *arg) {
    if (!pool || !fn) return false;
    cp_mutex_lock(&pool->lock);
    if (pool->queue_count == pool->queue_capacity || pool->shutdown) {
        cp_mutex_unlock(&pool->lock);
        return false;
    }

    pool->queue[pool->queue_tail].fn = fn;
    pool->queue[pool->queue_tail].arg = arg;
    pool->queue_tail = (pool->queue_tail + 1) % pool->queue_capacity;
    pool->queue_count++;
    cp_cond_signal(&pool->not_empty);
    cp_mutex_unlock(&pool->lock);
    return true;
}

void cp_threadpool_wait(cp_threadpool_t *pool) {
    if (!pool) return;
    cp_mutex_lock(&pool->lock);
    while ((pool->queue_count > 0 || pool->active_workers > 0) && !pool->shutdown) {
        cp_cond_wait(&pool->all_idle, &pool->lock);
    }
    cp_mutex_unlock(&pool->lock);
}

void cp_threadpool_destroy(cp_threadpool_t *pool) {
    if (!pool) return;
    cp_mutex_lock(&pool->lock);
    pool->shutdown = true;
    cp_cond_broadcast(&pool->not_empty);
    cp_cond_broadcast(&pool->not_full);
    cp_mutex_unlock(&pool->lock);

    for (usize i = 0; i < pool->num_threads; ++i) {
        pthread_join(pool->threads[i], NULL);
    }

    cp_mutex_destroy(&pool->lock);
    cp_cond_destroy(&pool->not_empty);
    cp_cond_destroy(&pool->not_full);
    cp_cond_destroy(&pool->all_idle);

    free(pool->threads);
    free(pool->queue);
    free(pool);
}

usize cp_threadpool_num_threads(const cp_threadpool_t *pool) {
    return pool ? pool->num_threads : 1;
}

/* -------------------------------------------------------------------------
 * Parallel-For Chunk Execution Engine
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_parallel_for_fn fn;
    usize start;
    usize end;
    void *arg;
    usize *remaining;
    cp_mutex_t *lock;
    cp_cond_t *cond;
} cp_parallel_chunk_t;

static void _cp_parallel_worker_task(void *raw) {
    cp_parallel_chunk_t *chunk = (cp_parallel_chunk_t *)raw;
    chunk->fn(chunk->start, chunk->end, chunk->arg);
    cp_mutex_lock(chunk->lock);
    (*chunk->remaining)--;
    if (*chunk->remaining == 0) {
        cp_cond_signal(chunk->cond);
    }
    cp_mutex_unlock(chunk->lock);
}

void cp_threadpool_parallel_for(cp_threadpool_t *pool, usize count, cp_parallel_for_fn fn, void *arg) {
    if (count == 0 || !fn) return;
    if (!pool || pool->num_threads <= 1 || count == 1) {
        fn(0, count, arg);
        return;
    }

    usize k = pool->num_threads;
    if (k > count) k = count;
    if (k > 128) k = 128;

    cp_parallel_chunk_t chunks[128];
    usize remaining = k;
    cp_mutex_t lock;
    cp_mutex_init(&lock);
    cp_cond_t cond;
    cp_cond_init(&cond);

    usize base_chunk = count / k;
    usize rem = count % k;
    usize cur_start = 0;

    for (usize i = 0; i < k; ++i) {
        usize slice = base_chunk + (i < rem ? 1 : 0);
        chunks[i].fn = fn;
        chunks[i].start = cur_start;
        chunks[i].end = cur_start + slice;
        chunks[i].arg = arg;
        chunks[i].remaining = &remaining;
        chunks[i].lock = &lock;
        chunks[i].cond = &cond;
        cur_start += slice;

        if (i < k - 1) {
            cp_threadpool_submit(pool, _cp_parallel_worker_task, &chunks[i]);
        }
    }

    /* Current calling thread processes chunk k - 1 directly */
    chunks[k - 1].fn(chunks[k - 1].start, chunks[k - 1].end, chunks[k - 1].arg);

    cp_mutex_lock(&lock);
    remaining--;
    while (remaining > 0) {
        cp_cond_wait(&cond, &lock);
    }
    cp_mutex_unlock(&lock);

    cp_mutex_destroy(&lock);
    cp_cond_destroy(&cond);
}
