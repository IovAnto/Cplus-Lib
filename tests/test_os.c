/**
 * @file test_os.c
 * @brief Dedicated unit tests for Operating System & Process utilities (cp_os).
 */

#include "cplus/cplus.h"
#include <string.h>
#include <assert.h>

static void test_os_system_info(void) {
    u32 cpus = OsNumCpus();
    assert(cpus >= 1);
    CP_UNUSED(cpus);

    u64 ram = OsTotalRamBytes();
    assert(ram > 0);
    CP_UNUSED(ram);

    u64 avail_ram = OsAvailRamBytes();
    assert(avail_ram > 0);
    assert(avail_ram <= ram);
    CP_UNUSED(avail_ram);

    usize page_size = OsPageSize();
    assert(page_size >= 4096);
    CP_UNUSED(page_size);

    char hostname[256];
    bool host_ok = OsHostname(hostname, sizeof(hostname));
    assert(host_ok);
    assert(strlen(hostname) > 0);
    CP_UNUSED(host_ok);

    u64 uptime = OsUptimeSec();
    assert(uptime > 0);
    CP_UNUSED(uptime);

    i32 pid = OsPid();
    assert(pid > 0);
    assert(OsIsRunning(pid));
    CP_UNUSED(pid);

    i32 ppid = OsPpid();
    assert(ppid > 0);
    CP_UNUSED(ppid);

    /* Non-existent PID should return false */
    assert(!OsIsRunning(2999999));
}

static void test_os_env_and_exec(void) {
    /* Environment variables */
    assert(OsEnvSet("CPLUS_TEST_KEY", "98765"));
    assert(OsEnvHas("CPLUS_TEST_KEY"));
    assert(strcmp(OsEnvGet("CPLUS_TEST_KEY"), "98765") == 0);
    assert(OsEnvGetInt("CPLUS_TEST_KEY", 0) == 98765);

    assert(strcmp(OsEnvGetOr("CPLUS_NONEXISTENT_VAR_123", "default_val"), "default_val") == 0);
    assert(OsEnvGetInt("CPLUS_NONEXISTENT_VAR_123", 42) == 42);

    assert(OsEnvUnset("CPLUS_TEST_KEY"));
    assert(!OsEnvHas("CPLUS_TEST_KEY"));
    assert(OsEnvGet("CPLUS_TEST_KEY") == NULL);

    /* OsExec */
    char out[256];
    int exit_code = -1;
    i64 n = OsExec("echo CPLUS_OS_EXEC_OK", out, sizeof(out), &exit_code);
    assert(n > 0);
    assert(exit_code == 0);
    assert(strstr(out, "CPLUS_OS_EXEC_OK") != NULL);
    CP_UNUSED(n);

    /* Dot-notation */
    assert(os.pid() == OsPid());
    assert(os.numCpus() == OsNumCpus());

    /* High-resolution sleep without error */
    os.sleepUs(100);
}

int main(void) {
    printf("Running test_os...\n");
    test_os_system_info();
    test_os_env_and_exec();
    printf("All test_os unit tests passed successfully!\n");
    return 0;
}
