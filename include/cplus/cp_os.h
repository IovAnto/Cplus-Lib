/**
 * @file cp_os.h
 * @brief Zero-latency Operating System, Process, Hardware, and Environment utilities.
 */

#ifndef CPLUS_OS_H
#define CPLUS_OS_H

#include "cp_core.h"

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

#endif /* CPLUS_OS_H */
