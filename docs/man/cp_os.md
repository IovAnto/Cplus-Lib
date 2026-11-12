# Module Specification: cp_os (Operating System & Process Subsystem)

## 1. TL;DR
The `cp_os` subsystem provides portable, low-overhead system queries, process execution, environment variable management, and sub-millisecond sleep routines without external dependencies.

```c
#include "cplus/cplus.h"

/* 1. Inspect Hardware & Environment */
u32 cores = OsNumCpus();
u64 ram_mb = OsTotalRamBytes() / (1024 * 1024);
const char *user = OsEnvGetOr("USER", "unknown");

/* 2. Execute Subprocess & Capture Output */
char output[512];
int exit_code = 0;
i64 bytes = OsExec("uname -r", output, sizeof(output), &exit_code);

/* 3. High-Precision Sleep */
OsSleepMs(10);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Process Execution Model
`OsExec` runs child processes through synchronous pipe redirection:
```c
i64 cp_os_exec(const char *cmd, char *out_buf, usize out_cap, int *exit_code);
```
- Spawns a shell subshell via `popen()` in read mode.
- Streams standard output incrementally into the destination buffer `out_buf` up to `out_cap - 1` bytes.
- Null-terminates the resulting buffer and stores the child exit code via `pclose()`.
- Returns the total bytes written, or `-1` on fork/execution failure.

### 2.2 Hardware Query Internals
- **CPU Cores (`OsNumCpus`)**: Evaluates `sysconf(_SC_NPROCESSORS_ONLN)` on POSIX systems or `GetSystemInfo` on Windows.
- **Physical RAM (`OsTotalRamBytes`, `OsAvailRamBytes`)**: Evaluates `sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE)` on Linux/POSIX or `GlobalMemoryStatusEx` on Windows.
- **Page Size (`OsPageSize`)**: Queries system virtual memory page boundary (typically 4096 bytes).
- **Process Identifiers (`OsPid`, `OsPpid`)**: Direct bindings to `getpid()` and `getppid()`.

### 2.3 Non-Busy Sleep
- `OsSleepMs(u32 ms)` and `OsSleepUs(u64 us)` translate directly to `nanosleep()` on POSIX platforms, yielding execution time slices back to the kernel scheduler without high CPU spin cycles.

---

## 3. Operational Guarantees & Contracts

| Metric | Hardware Queries | Subprocess Exec | Env Access | Non-Busy Sleep |
|---|---|---|---|---|
| **Time Complexity** | $O(1)$ | $O(T)$ child execution time | $O(K)$ hash/table lookup | $O(\Delta t)$ |
| **Heap Allocations** | 0 | Internal libc pipe buffer | 0 | 0 |
| **Memory Safety** | Buffer size bounded | Null-terminated string guarantee | Returns immutable slice/copy | Safe |
| **Thread Safety** | Thread-safe | Thread-safe (POSIX `popen`) | Read thread-safe, write requires synchronization | Thread-safe |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Run Subprocess | `OsExec(cmd, buf, cap, code)` | `os.exec(cmd, buf, cap, code)` | Synchronous command execution with output capture |
| CPU Core Count | `OsNumCpus()` | `os.numCpus()` | Count of online logical processor cores |
| Total RAM | `OsTotalRamBytes()` | `os.totalRamBytes()` | Total physical system RAM in bytes |
| Available RAM | `OsAvailRamBytes()` | `os.availRamBytes()` | Free/available physical system RAM in bytes |
| Page Size | `OsPageSize()` | `os.pageSize()` | Virtual memory page size in bytes |
| Hostname | `OsHostname(buf, len)` | `os.hostname(buf, len)` | System network node hostname |
| Uptime | `OsUptimeSec()` | `os.uptimeSec()` | System uptime in seconds |
| Process ID | `OsPid()` | `os.pid()` | Current process identifier |
| Parent PID | `OsPpid()` | `os.ppid()` | Parent process identifier |
| Send Signal | `OsKill(pid, sig)` | `os.kill(pid, sig)` | Dispatch POSIX signal to target PID |
| Process Alive | `OsIsRunning(pid)` | `os.isRunning(pid)` | Check if target PID exists and responds |
| Get Env | `OsEnvGet(name)` | `os.envGet(name)` | Fetch environment variable value or NULL |
| Get Env Default | `OsEnvGetOr(name, def)` | `os.envGetOr(name, def)` | Fetch environment variable or fallback string |
| Get Env Int | `OsEnvGetInt(name, def)` | `os.envGetInt(name, def)` | Parse integer environment variable with fallback |
| Set Env | `OsEnvSet(name, val)` | `os.envSet(name, val)` | Modify or create environment variable |
| Unset Env | `OsEnvUnset(name)` | `os.envUnset(name)` | Remove environment variable |
| Has Env | `OsEnvHas(name)` | `os.envHas(name)` | Check existence of environment variable |
| Sleep (ms) | `OsSleepMs(ms)` | `os.sleepMs(ms)` | Pause execution for millisecond duration |
| Sleep (us) | `OsSleepUs(us)` | `os.sleepUs(us)` | Pause execution for microsecond duration |

---

## 5. Use Cases
1. **Dynamic Thread Pool Sizing**: Automatically size thread pools at application startup based on online processor count (`OsNumCpus()`).
2. **System Health Monitors**: Track host memory consumption and system uptime inside daemon health checks.
3. **Subprocess Management**: Execute external command-line utilities, compilers, or diagnostics and parse output into caller-allocated buffers.
4. **Environment-Driven Configuration**: Read operational ports, log levels, or configuration tokens with fallback defaults (`OsEnvGetInt("PORT", 8080)`).

---

## 6. Complete Compilable Example

```c
#include "cplus/cplus.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    /* 1. Host hardware diagnostics */
    u32 cores = os.numCpus();
    u64 total_ram = os.totalRamBytes();
    u64 avail_ram = os.availRamBytes();
    usize page_sz = os.pageSize();

    printf("Hardware: %u Cores | RAM: %llu MB Total / %llu MB Available | Page: %zu B\n",
           cores,
           (unsigned long long)(total_ram / (1024 * 1024)),
           (unsigned long long)(avail_ram / (1024 * 1024)),
           page_sz);

    /* 2. Process inspection */
    i32 pid = os.pid();
    i32 ppid = os.ppid();
    bool alive = os.isRunning(pid);
    assert(alive == true);
    printf("PID: %d (Parent: %d) Alive: %d\n", pid, ppid, alive);

    /* 3. Environment variable management */
    os.envSet("CPLUS_TEST_VAR", "active");
    assert(os.envHas("CPLUS_TEST_VAR") == true);
    assert(strcmp(os.envGet("CPLUS_TEST_VAR"), "active") == 0);

    i64 port = os.envGetInt("CPLUS_PORT", 9000);
    assert(port == 9000);

    os.envUnset("CPLUS_TEST_VAR");
    assert(os.envHas("CPLUS_TEST_VAR") == false);

    /* 4. Subprocess execution */
    char buf[256];
    int code = 0;
    i64 bytes = os.exec("echo -n 'hello world'", buf, sizeof(buf), &code);
    assert(bytes > 0);
    assert(code == 0);
    assert(strcmp(buf, "hello world") == 0);

    return 0;
}
```

---

## 7. Limitations & Edge Cases
- **Command Shell Dependency**: `OsExec` invokes `/bin/sh -c` on POSIX systems. Ensure untrusted inputs are sanitized to avoid shell injection vulnerabilities.
- **Process Output Buffering**: If subprocess output exceeds the capacity of `out_buf`, trailing bytes are discarded. To process arbitrary streams, consume data via custom pipes or temporary files.
- **Environment Mutability**: `OsEnvSet` is not thread-safe if other threads call `getenv()` concurrently without a shared lock.
