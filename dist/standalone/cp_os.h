/**
 * @file cp_os.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_os.
 *
 * In ONE file define:
 *   #define CPLUS_CP_OS_IMPLEMENTATION
 *   #include "cp_os.h"
 */
#ifndef CPLUS_CP_OS_H
#define CPLUS_CP_OS_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
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
 * @file cp_os.h
 * @brief Zero-latency Operating System, Process, Hardware, and Environment utilities.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Process Execution & System Info
 * ------------------------------------------------------------------------- */

/**
 * @brief Executes a shell command synchronously and captures standard output.
 * @param cmd The shell command line to execute.
 * @param out_buf Buffer to store standard output (NULL-terminated).
 * @param out_cap Maximum capacity of out_buf.
 * @param exit_code Pointer to store the exit code of the process (can be NULL).
 * @return Number of bytes written to out_buf (excluding NULL terminator), or -1 on error.
 */
i64 cp_os_exec(const char *cmd, char *out_buf, usize out_cap, int *exit_code);

/**
 * @brief Returns the number of logical CPU cores online.
 */
u32 cp_os_num_cpus(void);

/**
 * @brief Returns total physical RAM in bytes.
 */
u64 cp_os_total_ram_bytes(void);

/**
 * @brief Returns currently available physical RAM in bytes.
 */
u64 cp_os_avail_ram_bytes(void);

/**
 * @brief Returns system virtual memory page size in bytes.
 */
usize cp_os_page_size(void);

/**
 * @brief Gets the system hostname.
 * @param buf Output buffer.
 * @param len Buffer length.
 * @return True if successfully retrieved, false otherwise.
 */
bool cp_os_hostname(char *buf, usize len);

/**
 * @brief Returns system uptime in seconds.
 */
u64 cp_os_uptime_sec(void);

/* -------------------------------------------------------------------------
 * Process Management & Signals
 * ------------------------------------------------------------------------- */

/**
 * @brief Returns the current process ID (PID).
 */
i32 cp_os_pid(void);

/**
 * @brief Returns the parent process ID (PPID).
 */
i32 cp_os_ppid(void);

/**
 * @brief Sends a signal to a process.
 * @param pid Process ID.
 * @param sig Signal number (e.g., SIGTERM, SIGKILL, 0 to test existence).
 * @return True on success, false on error.
 */
bool cp_os_kill(i32 pid, int sig);

/**
 * @brief Checks if a process with given PID is alive.
 */
bool cp_os_is_process_running(i32 pid);

/* -------------------------------------------------------------------------
 * Environment Variables
 * ------------------------------------------------------------------------- */

/**
 * @brief Gets an environment variable string, or NULL if unset.
 */
const char *cp_os_env_get(const char *name);

/**
 * @brief Gets an environment variable string, returning default_val if unset.
 */
const char *cp_os_env_get_or(const char *name, const char *default_val);

/**
 * @brief Gets an environment variable parsed as an integer, returning default_val if unset or invalid.
 */
i64 cp_os_env_get_int(const char *name, i64 default_val);

/**
 * @brief Sets an environment variable. Overwrites if already exists.
 * @return True on success, false on failure.
 */
bool cp_os_env_set(const char *name, const char *value);

/**
 * @brief Unsets an environment variable.
 * @return True on success, false on failure.
 */
bool cp_os_env_unset(const char *name);

/**
 * @brief Checks if an environment variable exists.
 */
bool cp_os_env_has(const char *name);

/* -------------------------------------------------------------------------
 * High-Resolution Sleep
 * ------------------------------------------------------------------------- */

/**
 * @brief Sleeps for the specified number of milliseconds without busy-waiting.
 */
void cp_os_sleep_ms(u32 ms);

/**
 * @brief Sleeps for the specified number of microseconds without busy-waiting.
 */
void cp_os_sleep_us(u64 us);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define OsExec(cmd, buf, cap, exit_code) cp_os_exec(cmd, buf, cap, exit_code)
#define OsNumCpus()                      cp_os_num_cpus()
#define OsTotalRamBytes()                cp_os_total_ram_bytes()
#define OsAvailRamBytes()                cp_os_avail_ram_bytes()
#define OsPageSize()                     cp_os_page_size()
#define OsHostname(buf, len)             cp_os_hostname(buf, len)
#define OsUptimeSec()                    cp_os_uptime_sec()

#define OsPid()                          cp_os_pid()
#define OsPpid()                         cp_os_ppid()
#define OsKill(pid, sig)                 cp_os_kill(pid, sig)
#define OsIsRunning(pid)                 cp_os_is_process_running(pid)

#define OsEnvGet(name)                   cp_os_env_get(name)
#define OsEnvGetOr(name, def)            cp_os_env_get_or(name, def)
#define OsEnvGetInt(name, def)           cp_os_env_get_int(name, def)
#define OsEnvSet(name, val)              cp_os_env_set(name, val)
#define OsEnvUnset(name)                 cp_os_env_unset(name)
#define OsEnvHas(name)                   cp_os_env_has(name)

#define OsSleepMs(ms)                    cp_os_sleep_ms(ms)
#define OsSleepUs(us)                    cp_os_sleep_us(us)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    i64 (*const exec)(const char *cmd, char *out_buf, usize out_cap, int *exit_code);
    u32 (*const numCpus)(void);
    u64 (*const totalRamBytes)(void);
    u64 (*const availRamBytes)(void);
    usize (*const pageSize)(void);
    bool (*const hostname)(char *buf, usize len);
    u64 (*const uptimeSec)(void);

    i32 (*const pid)(void);
    i32 (*const ppid)(void);
    bool (*const kill)(i32 pid, int sig);
    bool (*const isRunning)(i32 pid);

    const char *(*const envGet)(const char *name);
    const char *(*const envGetOr)(const char *name, const char *default_val);
    i64 (*const envGetInt)(const char *name, i64 default_val);
    bool (*const envSet)(const char *name, const char *value);
    bool (*const envUnset)(const char *name);
    bool (*const envHas)(const char *name);

    void (*const sleepMs)(u32 ms);
    void (*const sleepUs)(u64 us);
} cp_os_namespace_t;

static const cp_os_namespace_t os = {
    .exec = cp_os_exec,
    .numCpus = cp_os_num_cpus,
    .totalRamBytes = cp_os_total_ram_bytes,
    .availRamBytes = cp_os_avail_ram_bytes,
    .pageSize = cp_os_page_size,
    .hostname = cp_os_hostname,
    .uptimeSec = cp_os_uptime_sec,
    .pid = cp_os_pid,
    .ppid = cp_os_ppid,
    .kill = cp_os_kill,
    .isRunning = cp_os_is_process_running,
    .envGet = cp_os_env_get,
    .envGetOr = cp_os_env_get_or,
    .envGetInt = cp_os_env_get_int,
    .envSet = cp_os_env_set,
    .envUnset = cp_os_env_unset,
    .envHas = cp_os_env_has,
    .sleepMs = cp_os_sleep_ms,
    .sleepUs = cp_os_sleep_us,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_OS_H */

#ifdef CPLUS_CP_OS_IMPLEMENTATION
/**
 * @file cp_os.c
 * @brief Zero-latency Operating System, Process, Hardware, and Environment utilities.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>

i64 cp_os_exec(const char *cmd, char *out_buf, usize out_cap, int *exit_code) {
    if (!cmd) return -1;

    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    usize total_read = 0;
    if (out_buf && out_cap > 0) {
        while (total_read + 1 < out_cap) {
            usize n = fread(out_buf + total_read, 1, out_cap - 1 - total_read, fp);
            if (n == 0) break;
            total_read += n;
        }
        out_buf[total_read] = '\0';

        /* Drain remainder if any */
        char drain[256];
        while (fread(drain, 1, sizeof(drain), fp) > 0) {
            /* discard */
        }
    } else {
        char drain[256];
        while (fread(drain, 1, sizeof(drain), fp) > 0) {
            /* discard */
        }
    }

    int status = pclose(fp);
    if (exit_code) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = 128 + WTERMSIG(status);
        } else {
            *exit_code = status;
        }
    }

    return (i64)total_read;
}

u32 cp_os_num_cpus(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (u32)n : 1;
}

u64 cp_os_total_ram_bytes(void) {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return (u64)pages * (u64)page_size;
    }
    return 0;
}

u64 cp_os_avail_ram_bytes(void) {
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return (u64)pages * (u64)page_size;
    }
    return 0;
}

usize cp_os_page_size(void) {
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (page_size > 0) ? (usize)page_size : 4096;
}

bool cp_os_hostname(char *buf, usize len) {
    if (!buf || len == 0) return false;
    if (gethostname(buf, len) == 0) {
        buf[len - 1] = '\0';
        return true;
    }
    return false;
}

u64 cp_os_uptime_sec(void) {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return (u64)info.uptime;
    }
    return 0;
}

i32 cp_os_pid(void) {
    return (i32)getpid();
}

i32 cp_os_ppid(void) {
    return (i32)getppid();
}

bool cp_os_kill(i32 pid, int sig) {
    return kill((pid_t)pid, sig) == 0;
}

bool cp_os_is_process_running(i32 pid) {
    if (pid <= 0) return false;
    if (kill((pid_t)pid, 0) == 0) return true;
    return (errno == EPERM);
}

const char *cp_os_env_get(const char *name) {
    if (!name) return NULL;
    return getenv(name);
}

const char *cp_os_env_get_or(const char *name, const char *default_val) {
    const char *val = cp_os_env_get(name);
    return val ? val : default_val;
}

i64 cp_os_env_get_int(const char *name, i64 default_val) {
    const char *val = cp_os_env_get(name);
    if (!val || *val == '\0') return default_val;
    char *endptr = NULL;
    long long num = strtoll(val, &endptr, 10);
    if (endptr == val) return default_val;
    return (i64)num;
}

bool cp_os_env_set(const char *name, const char *value) {
    if (!name) return false;
    return setenv(name, value ? value : "", 1) == 0;
}

bool cp_os_env_unset(const char *name) {
    if (!name) return false;
    return unsetenv(name) == 0;
}

bool cp_os_env_has(const char *name) {
    return cp_os_env_get(name) != NULL;
}

void cp_os_sleep_ms(u32 ms) {
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

void cp_os_sleep_us(u64 us) {
    struct timespec req;
    req.tv_sec = (time_t)(us / 1000000ULL);
    req.tv_nsec = (long)(us % 1000000ULL) * 1000L;
    nanosleep(&req, NULL);
}
#endif /* CPLUS_CP_OS_IMPLEMENTATION */
