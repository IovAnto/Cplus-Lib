/**
 * @file cp_os.c
 * @brief Zero-latency Operating System, Process, Hardware, and Environment utilities.
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "cplus/cp_os.h"
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
