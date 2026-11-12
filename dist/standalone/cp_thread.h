/**
 * @file cp_thread.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_thread.
 *
 * In ONE file define:
 *   #define CPLUS_CP_THREAD_IMPLEMENTATION
 *   #include "cp_thread.h"
 */
#ifndef CPLUS_CP_THREAD_H
#define CPLUS_CP_THREAD_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Core types dependency */
#ifndef CPLUS_CORE_H
#define CPLUS_CORE_H
/**
 * @file cp_core.h
 * @brief Foundational types, platform macros, RAII defer, and polymorphic printing.
 */


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <alloca.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Exact-width & ergonomic type aliases
 * ------------------------------------------------------------------------- */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef float     f32;
typedef double    f64;

typedef uint8_t   b8;
typedef uint32_t  b32;

/* -------------------------------------------------------------------------
 * Compiler & Optimization Attributes
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) || defined(__clang__)
    #define CP_INLINE static inline __attribute__((always_inline))
    #define CP_NOINLINE __attribute__((noinline))
    #define CP_LIKELY(x) __builtin_expect(!!(x), 1)
    #define CP_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define CP_ALIGN(n) __attribute__((aligned(n)))
    #define CP_UNUSED(x) ((void)(x))
#else
    #define CP_INLINE static inline
    #define CP_NOINLINE
    #define CP_LIKELY(x) (x)
    #define CP_UNLIKELY(x) (x)
    #define CP_ALIGN(n)
    #define CP_UNUSED(x) ((void)(x))
#endif

#define CP_CACHELINE_SIZE 64
#define CP_CACHELINE CP_ALIGN(CP_CACHELINE_SIZE)

/* Default SIMD alignment */
#define CP_DEFAULT_ALIGNMENT 16
#define CP_SIMD_ALIGNMENT    32

/* Helper macro token pasting */
#define CP_CONCAT_INTERNAL_(a, b) a ## b
#define CP_CONCAT(a, b) CP_CONCAT_INTERNAL_(a, b)

/* -------------------------------------------------------------------------
 * RAII Defer mechanism
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*fn)(void *);
    void *arg;
} cp_defer_t;

CP_INLINE void _cp_defer_cleanup(cp_defer_t *d) {
    if (d->fn) {
        d->fn(d->arg);
    }
}

typedef void (*cp_action_fn)(void);
CP_INLINE void _cp_defer_action_cleanup(cp_action_fn *fn) {
    if (*fn) {
        (*fn)();
    }
}

/**
 * @brief Defer execution of a function with 1 argument until the enclosing scope exits.
 * @param func_ Pointer to function taking 1 pointer argument: void func(void *arg)
 * @param arg_ Pointer passed to func when exiting scope.
 */
#define Defer(func_, arg_) \
    __attribute__((cleanup(_cp_defer_cleanup))) cp_defer_t CP_CONCAT(_cp_defer_, __LINE__) = { \
        .fn = (void (*)(void *))(func_), \
        .arg = (void *)(arg_) \
    }

/**
 * @brief Defer execution of a parameterless function until the enclosing scope exits.
 */
#define DeferAction(func_) \
    __attribute__((cleanup(_cp_defer_action_cleanup))) cp_action_fn CP_CONCAT(_cp_defact_, __LINE__) = (func_)

/* -------------------------------------------------------------------------
 * Anonymous Lambda Functions (Zero Runtime Overhead)
 * Supported natively on GCC via GNU C extension ({ ret_type fn args body fn; }).
 * On Clang / MSVC without nested function syntax, use static functions or
 * zero-overhead iteration macros (VecEach, VecMap).
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) && !defined(__clang__) && !defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__)
#define CP_HAS_LAMBDA 1
/**
 * @brief Define an anonymous inlined lambda function with zero runtime overhead (GCC).
 * @example AlgoSort(arr, len, sizeof(int), Lambda(int, (const void *a, const void *b), { return *(int*)a - *(int*)b; }));
 */
#define Lambda(ret_type, args, body) __extension__ ({ ret_type _cp_lambda args body _cp_lambda; })
#else
#define CP_HAS_LAMBDA 0
#endif

/* -------------------------------------------------------------------------
 * Polymorphic Type-Safe Printing (C11 _Generic)
 * ------------------------------------------------------------------------- */
CP_INLINE void _cp_print_bool(bool v) { fputs(v ? "true" : "false", stdout); }
CP_INLINE void _cp_print_char(char v) { putchar(v); }
CP_INLINE void _cp_print_i64(long long v) { printf("%lld", v); }
CP_INLINE void _cp_print_u64(unsigned long long v) { printf("%llu", v); }
CP_INLINE void _cp_print_f64(double v) { printf("%.6g", v); }
CP_INLINE void _cp_print_str(const char *v) { fputs(v ? v : "(null)", stdout); }
CP_INLINE void _cp_print_ptr(const void *v) { printf("%p", v); }

#define Print(val) _Generic((val), \
    _Bool: _cp_print_bool, \
    char: _cp_print_char, \
    signed char: _cp_print_i64, \
    short: _cp_print_i64, \
    int: _cp_print_i64, \
    long: _cp_print_i64, \
    long long: _cp_print_i64, \
    unsigned char: _cp_print_u64, \
    unsigned short: _cp_print_u64, \
    unsigned int: _cp_print_u64, \
    unsigned long: _cp_print_u64, \
    unsigned long long: _cp_print_u64, \
    float: _cp_print_f64, \
    double: _cp_print_f64, \
    char*: _cp_print_str, \
    const char*: _cp_print_str, \
    default: _cp_print_ptr \
)(val)

#define Println(val) do { Print(val); putchar('\n'); } while(0)
#define PrintF(fmt, ...) printf(fmt, __VA_ARGS__)

/* -------------------------------------------------------------------------
 * Panic & Assertions
 * ------------------------------------------------------------------------- */
#define Panic(msg) do { \
    fprintf(stderr, "\n[CPLUS PANIC] %s:%d: %s\n", __FILE__, __LINE__, msg); \
    abort(); \
} while(0)

#define AssertMsg(cond, msg) do { \
    if (CP_UNLIKELY(!(cond))) { \
        fprintf(stderr, "\n[CPLUS ASSERTION FAILED] %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, msg); \
        abort(); \
    } \
} while(0)

/* -------------------------------------------------------------------------
 * Bit Manipulation Intrinsics
 * ------------------------------------------------------------------------- */
CP_INLINE u32 cp_bit_popcount32(u32 x) { return (u32)__builtin_popcount(x); }
CP_INLINE u32 cp_bit_popcount64(u64 x) { return (u32)__builtin_popcountll(x); }
CP_INLINE u32 cp_bit_clz32(u32 x) { return x == 0 ? 32 : (u32)__builtin_clz(x); }
CP_INLINE u32 cp_bit_clz64(u64 x) { return x == 0 ? 64 : (u32)__builtin_clzll(x); }
CP_INLINE u32 cp_bit_ctz32(u32 x) { return x == 0 ? 32 : (u32)__builtin_ctz(x); }
CP_INLINE u32 cp_bit_ctz64(u64 x) { return x == 0 ? 64 : (u32)__builtin_ctzll(x); }
CP_INLINE u16 cp_bit_bswap16(u16 x) { return __builtin_bswap16(x); }
CP_INLINE u32 cp_bit_bswap32(u32 x) { return __builtin_bswap32(x); }
CP_INLINE u64 cp_bit_bswap64(u64 x) { return __builtin_bswap64(x); }
CP_INLINE u32 cp_bit_rotl32(u32 x, u32 s) { return (x << (s & 31)) | (x >> ((32 - (s & 31)) & 31)); }
CP_INLINE u32 cp_bit_rotr32(u32 x, u32 s) { return (x >> (s & 31)) | (x << ((32 - (s & 31)) & 31)); }
CP_INLINE u64 cp_bit_rotl64(u64 x, u32 s) { return (x << (s & 63)) | (x >> ((64 - (s & 63)) & 63)); }
CP_INLINE u64 cp_bit_rotr64(u64 x, u32 s) { return (x >> (s & 63)) | (x << ((64 - (s & 63)) & 63)); }
CP_INLINE bool cp_bit_is_pow2(u64 x) { return (x != 0) && ((x & (x - 1)) == 0); }
CP_INLINE u64 cp_bit_next_pow2(u64 x) {
    if (x <= 1) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

#define BitPopcount32(x) cp_bit_popcount32(x)
#define BitPopcount64(x) cp_bit_popcount64(x)
#define BitClz32(x)      cp_bit_clz32(x)
#define BitClz64(x)      cp_bit_clz64(x)
#define BitCtz32(x)      cp_bit_ctz32(x)
#define BitCtz64(x)      cp_bit_ctz64(x)
#define BitBswap16(x)    cp_bit_bswap16(x)
#define BitBswap32(x)    cp_bit_bswap32(x)
#define BitBswap64(x)    cp_bit_bswap64(x)
#define BitRotl32(x, s)  cp_bit_rotl32(x, s)
#define BitRotr32(x, s)  cp_bit_rotr32(x, s)
#define BitRotl64(x, s)  cp_bit_rotl64(x, s)
#define BitRotr64(x, s)  cp_bit_rotr64(x, s)
#define BitIsPow2(x)     cp_bit_is_pow2(x)
#define BitNextPow2(x)   cp_bit_next_pow2(x)

typedef struct {
    u32 (*const popcount32)(u32 x);
    u32 (*const popcount64)(u64 x);
    u32 (*const clz32)(u32 x);
    u32 (*const clz64)(u64 x);
    u32 (*const ctz32)(u32 x);
    u32 (*const ctz64)(u64 x);
    u16 (*const bswap16)(u16 x);
    u32 (*const bswap32)(u32 x);
    u64 (*const bswap64)(u64 x);
    u32 (*const rotl32)(u32 x, u32 s);
    u32 (*const rotr32)(u32 x, u32 s);
    u64 (*const rotl64)(u64 x, u32 s);
    u64 (*const rotr64)(u64 x, u32 s);
    bool (*const isPow2)(u64 x);
    u64 (*const nextPow2)(u64 x);
} cp_bit_namespace_t;

static const cp_bit_namespace_t bit = {
    .popcount32 = cp_bit_popcount32,
    .popcount64 = cp_bit_popcount64,
    .clz32 = cp_bit_clz32,
    .clz64 = cp_bit_clz64,
    .ctz32 = cp_bit_ctz32,
    .ctz64 = cp_bit_ctz64,
    .bswap16 = cp_bit_bswap16,
    .bswap32 = cp_bit_bswap32,
    .bswap64 = cp_bit_bswap64,
    .rotl32 = cp_bit_rotl32,
    .rotr32 = cp_bit_rotr32,
    .rotl64 = cp_bit_rotl64,
    .rotr64 = cp_bit_rotr64,
    .isPow2 = cp_bit_is_pow2,
    .nextPow2 = cp_bit_next_pow2,
};

/* -------------------------------------------------------------------------
 * Colored Diagnostic Logging
 * ------------------------------------------------------------------------- */
typedef enum {
    CP_LOG_LEVEL_DEBUG = 0,
    CP_LOG_LEVEL_INFO  = 1,
    CP_LOG_LEVEL_WARN  = 2,
    CP_LOG_LEVEL_ERROR = 3,
    CP_LOG_LEVEL_OFF   = 4,
} cp_log_level_t;

static inline cp_log_level_t *_cp_log_level_ptr(void) {
    static cp_log_level_t lvl = CP_LOG_LEVEL_INFO;
    return &lvl;
}

CP_INLINE void cp_log_set_level(cp_log_level_t lvl) {
    *_cp_log_level_ptr() = lvl;
}

CP_INLINE cp_log_level_t cp_log_get_level(void) {
    return *_cp_log_level_ptr();
}

#define LogSetLevel(lvl) cp_log_set_level(lvl)

#define LogDebug(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_DEBUG) { \
        fprintf(stderr, "\033[90m[DEBUG]\033[0m " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogInfo(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_INFO) { \
        fprintf(stdout, "\033[32m[INFO]\033[0m  " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogWarn(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_WARN) { \
        fprintf(stderr, "\033[33m[WARN]\033[0m  " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogError(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_ERROR) { \
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif
#endif /* CPLUS_CORE_H */

/**
 * @file cp_thread.h
 * @brief Zero-overhead threading primitives, scoped RAII locks, and lock-free/low-latency thread pool.
 */


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

#endif /* CPLUS_CP_THREAD_H */

#ifdef CPLUS_CP_THREAD_IMPLEMENTATION
/**
 * @file cp_thread.c
 * @brief Implementation of low-latency worker thread pool and threading utilities.
 */

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
#endif /* CPLUS_CP_THREAD_IMPLEMENTATION */
