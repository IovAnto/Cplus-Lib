/**
 * @file test_thread_net.c
 * @brief Comprehensive unit test suite for cp_thread and cp_net modules.
 */

#include "cplus/cplus.h"
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#define TEST_PORT_TCP 29124
#define TEST_PORT_UDP 29125

/* -------------------------------------------------------------------------
 * Test 1: Thread Creation, Join, Self & Yield
 * ------------------------------------------------------------------------- */
static void *thread_worker_sum(void *arg) {
    u64 *limit = (u64 *)arg;
    u64 *res = (u64 *)malloc(sizeof(u64));
    *res = 0;
    for (u64 i = 1; i <= *limit; ++i) {
        *res += i;
    }
    ThreadYield();
    return res;
}

static void test_thread_basic(void) {
    printf("[TEST] cp_thread: basic thread creation, self and join...\n");

    cp_thread_t self = ThreadSelf();
    CP_UNUSED(self);

    cp_thread_t worker;
    u64 n = 1000;
    bool ok = ThreadCreate(&worker, thread_worker_sum, &n);
    assert(ok);
    CP_UNUSED(ok);

    void *ret_val = NULL;
    ok = ThreadJoin(worker, &ret_val);
    assert(ok);
    assert(ret_val != NULL);

    u64 sum = *(u64 *)ret_val;
    free(ret_val);
    assert(sum == (1000ULL * 1001ULL) / 2ULL);
    CP_UNUSED(sum);

    printf("       Thread sum calculated correctly: %llu\n", (unsigned long long)sum);
}

/* -------------------------------------------------------------------------
 * Test 2: Mutex, ScopedLock, and WithLock
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_mutex_t mtx;
    u64 counter;
} shared_counter_t;

static void *thread_scoped_lock_worker(void *arg) {
    shared_counter_t *sc = (shared_counter_t *)arg;
    for (int i = 0; i < 10000; ++i) {
        {
            ScopedLock(&sc->mtx);
            sc->counter++;
        }
    }
    return NULL;
}

static void *thread_with_lock_worker(void *arg) {
    shared_counter_t *sc = (shared_counter_t *)arg;
    for (int i = 0; i < 10000; ++i) {
        WithLock(&sc->mtx) {
            sc->counter++;
        }
    }
    return NULL;
}

static void test_mutex_and_scoped_lock(void) {
    printf("[TEST] cp_thread: Mutex, ScopedLock RAII, and WithLock block scoping...\n");

    shared_counter_t sc;
    sc.counter = 0;
    MutexInit(&sc.mtx);

    assert(MutexTryLock(&sc.mtx));
    MutexUnlock(&sc.mtx);

    #define NUM_WORKERS 4
    cp_thread_t workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; ++i) {
        if (i % 2 == 0) {
            ThreadCreate(&workers[i], thread_scoped_lock_worker, &sc);
        } else {
            ThreadCreate(&workers[i], thread_with_lock_worker, &sc);
        }
    }

    for (int i = 0; i < NUM_WORKERS; ++i) {
        ThreadJoin(workers[i], NULL);
    }

    assert(sc.counter == 40000);
    MutexDestroy(&sc.mtx);
    printf("       Contended counter accurately protected: %llu\n", (unsigned long long)sc.counter);
}

/* -------------------------------------------------------------------------
 * Test 3: Condition Variable & Timed Wait
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_mutex_t mtx;
    cp_cond_t cond;
    bool ready;
} cond_data_t;

static void *thread_cond_signaler(void *arg) {
    cond_data_t *cd = (cond_data_t *)arg;
    usleep(10000); /* 10 ms */
    MutexLock(&cd->mtx);
    cd->ready = true;
    CondSignal(&cd->cond);
    MutexUnlock(&cd->mtx);
    return NULL;
}

static void test_condvar(void) {
    printf("[TEST] cp_thread: Condition Variable signal/wait and timedwait...\n");

    cond_data_t cd;
    cd.ready = false;
    MutexInit(&cd.mtx);
    CondInit(&cd.cond);

    /* Test timedwait timeout */
    MutexLock(&cd.mtx);
    bool timed_out = !CondTimedWaitMs(&cd.cond, &cd.mtx, 15);
    MutexUnlock(&cd.mtx);
    assert(timed_out);
    CP_UNUSED(timed_out);

    /* Test signal & wait */
    cp_thread_t th;
    ThreadCreate(&th, thread_cond_signaler, &cd);

    MutexLock(&cd.mtx);
    while (!cd.ready) {
        CondWait(&cd.cond, &cd.mtx);
    }
    assert(cd.ready == true);
    MutexUnlock(&cd.mtx);

    ThreadJoin(th, NULL);
    MutexDestroy(&cd.mtx);
    CondDestroy(&cd.cond);
    printf("       CondVar timeout and signal/wait verified successfully.\n");
}

/* -------------------------------------------------------------------------
 * Test 4: Low-Latency Thread Pool
 * ------------------------------------------------------------------------- */
static volatile i64 g_pool_task_counter = 0;

static void pool_task_increment(void *arg) {
    i64 increment = (i64)(intptr_t)arg;
    AtomicAdd(&g_pool_task_counter, increment);
}

static void test_thread_pool(void) {
    printf("[TEST] cp_thread: ThreadPool creation, mass task dispatch, and wait...\n");

    g_pool_task_counter = 0;
    const usize task_count = 20000;
    cp_threadpool_t *p = ThreadPoolCreate(4, 4096);
    assert(p != NULL);

    for (usize i = 0; i < task_count; ++i) {
        bool ok = ThreadPoolSubmit(p, pool_task_increment, (void *)(intptr_t)1);
        assert(ok);
        CP_UNUSED(ok);
    }

    ThreadPoolWait(p);
    assert(AtomicLoad(&g_pool_task_counter) == (i64)task_count);

    /* Test TrySubmit */
    bool tried = ThreadPoolTrySubmit(p, pool_task_increment, (void *)(intptr_t)5);
    assert(tried);
    CP_UNUSED(tried);

    ThreadPoolWait(p);
    assert(AtomicLoad(&g_pool_task_counter) == (i64)(task_count + 5));

    ThreadPoolDestroy(p);
    printf("       ThreadPool executed %zu tasks with exact atomic count.\n", task_count + 5);
}

/* -------------------------------------------------------------------------
 * Test 5: TCP Loopback Server/Client & NetTcpRequest
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_socket_t listen_fd;
    volatile bool running;
} echo_server_t;

static void *echo_server_worker(void *arg) {
    echo_server_t *es = (echo_server_t *)arg;
    char client_ip[64];
    u16 client_port = 0;

    cp_socket_t client = NetTcpAccept(es->listen_fd, client_ip, sizeof(client_ip), &client_port);
    if (client >= 0) {
        char buf[256];
        i64 n = NetRecv(client, buf, sizeof(buf) - 1, 1000);
        if (n > 0) {
            buf[n] = '\0';
            /* Send echo back with prefix "ECHO:" */
            char reply[512];
            int rlen = snprintf(reply, sizeof(reply), "ECHO:%s", buf);
            NetSendAll(client, reply, (usize)rlen, 1000);
        }
        NetClose(client);
    }
    return NULL;
}

static void test_net_tcp_loopback(void) {
    printf("[TEST] cp_net: TCP listen, accept, send, recv, and loopback roundtrip...\n");

    cp_socket_t server_fd = NetTcpListen("127.0.0.1", TEST_PORT_TCP, 5);
    assert(server_fd >= 0);

    echo_server_t es = { .listen_fd = server_fd, .running = true };
    cp_thread_t srv_th;
    ThreadCreate(&srv_th, echo_server_worker, &es);

    usleep(10000); /* 10 ms wait for listen */

    /* Client connection */
    cp_socket_t client_fd = NetTcpConnect("127.0.0.1", TEST_PORT_TCP, 1000);
    assert(client_fd >= 0);

    const char *payload = "CPLUS_HIGH_SPEED";
    i64 sent = NetSendAll(client_fd, payload, strlen(payload), 1000);
    assert(sent == (i64)strlen(payload));
    CP_UNUSED(sent);

    char resp[256];
    memset(resp, 0, sizeof(resp));
    i64 recvd = NetRecv(client_fd, resp, sizeof(resp) - 1, 1000);
    assert(recvd > 0);
    CP_UNUSED(recvd);
    assert(strcmp(resp, "ECHO:CPLUS_HIGH_SPEED") == 0);

    NetClose(client_fd);
    ThreadJoin(srv_th, NULL);
    NetClose(server_fd);

    printf("       TCP roundtrip confirmed: received \"%s\"\n", resp);
}

/* -------------------------------------------------------------------------
 * Test 6: UDP Socket Bind & Send/Recv
 * ------------------------------------------------------------------------- */
static void test_net_udp(void) {
    printf("[TEST] cp_net: UDP bind, sendTo, and recvFrom loopback...\n");

    cp_socket_t srv = NetUdpBind("127.0.0.1", TEST_PORT_UDP);
    assert(srv >= 0);

    const char *msg = "UDP_TELEMETRY_PACKET";
    bool sent = NetUdpSend("127.0.0.1", TEST_PORT_UDP, msg, strlen(msg));
    assert(sent);
    CP_UNUSED(sent);

    char buf[128];
    memset(buf, 0, sizeof(buf));
    char sender_ip[64];
    u16 sender_port = 0;
    i64 n = NetUdpRecvFrom(srv, buf, sizeof(buf) - 1, sender_ip, sizeof(sender_ip), &sender_port, 1000);
    assert(n == (i64)strlen(msg));
    CP_UNUSED(n);
    assert(strcmp(buf, msg) == 0);
    assert(strcmp(sender_ip, "127.0.0.1") == 0);

    NetClose(srv);
    printf("       UDP packet successfully received from %s:%u\n", sender_ip, sender_port);
}

/* -------------------------------------------------------------------------
 * Test 7: Modbus RTU & TCP Framing and Hardware CRC16
 * ------------------------------------------------------------------------- */
static void test_modbus(void) {
    printf("[TEST] cp_net: Industrial Modbus RTU/TCP builders, parsers, and CRC16...\n");

    /* Known test vector: [0x01, 0x03, 0x00, 0x00, 0x00, 0x0A] -> CRC = 0xCDC5 */
    u8 vector[6] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x0A };
    u16 crc = ModbusCrc16(vector, 6);
    assert(crc == 0xCDC5);
    CP_UNUSED(crc);

    /* 1. Modbus RTU Read Holding Registers */
    u8 rtu_frame[32];
    usize len = ModbusRtuBuildRead(rtu_frame, sizeof(rtu_frame), 1, 0x0000, 10);
    assert(len == 8);
    assert(rtu_frame[0] == 0x01);
    assert(rtu_frame[1] == 0x03);
    assert(rtu_frame[6] == 0xC5); /* LSB */
    assert(rtu_frame[7] == 0xCD); /* MSB */
    assert(ModbusRtuVerifyCrc(rtu_frame, 8));

    /* 2. Modbus RTU Write Single Register */
    len = ModbusRtuBuildWriteSingle(rtu_frame, sizeof(rtu_frame), 2, 0x0010, 0x04D2);
    assert(len == 8);
    assert(rtu_frame[0] == 0x02);
    assert(rtu_frame[1] == 0x06);
    assert(ModbusRtuVerifyCrc(rtu_frame, 8));

    /* 3. Modbus RTU Write Multiple Registers */
    u16 vals[3] = { 0x1111, 0x2222, 0x3333 };
    len = ModbusRtuBuildWriteMultiple(rtu_frame, sizeof(rtu_frame), 1, 0x0020, 3, vals);
    assert(len == 9 + 6);
    assert(ModbusRtuVerifyCrc(rtu_frame, len));
    CP_UNUSED(len);

    /* 4. Modbus RTU Parse Read Response */
    u8 resp_rtu[11] = {
        0x01,       /* slave */
        0x03,       /* fc */
        0x06,       /* byte count = 3 regs * 2 bytes */
        0x00, 0x64, /* 100 */
        0x00, 0xC8, /* 200 */
        0x01, 0x2C, /* 300 */
        0x00, 0x00  /* CRC placeholder */
    };
    u16 resp_crc = ModbusCrc16(resp_rtu, 9);
    resp_rtu[9] = (u8)(resp_crc & 0xFF);
    resp_rtu[10] = (u8)((resp_crc >> 8) & 0xFF);

    u16 parsed_regs[4];
    u16 parsed_count = 0;
    bool parse_ok = ModbusRtuParseRead(resp_rtu, 11, 1, parsed_regs, 4, &parsed_count);
    assert(parse_ok);
    assert(parsed_count == 3);
    assert(parsed_regs[0] == 100);
    assert(parsed_regs[1] == 200);
    assert(parsed_regs[2] == 300);

    /* 5. Modbus TCP Build Read Holding Registers */
    u8 tcp_frame[32];
    len = ModbusTcpBuildRead(tcp_frame, sizeof(tcp_frame), 0x1234, 1, 50, 4);
    assert(len == 12);
    assert(tcp_frame[0] == 0x12 && tcp_frame[1] == 0x34); /* Tx ID */
    assert(tcp_frame[2] == 0x00 && tcp_frame[3] == 0x00); /* Proto */
    assert(tcp_frame[4] == 0x00 && tcp_frame[5] == 0x06); /* Len = 6 */
    assert(tcp_frame[6] == 0x01);                         /* Unit */
    assert(tcp_frame[7] == 0x03);                         /* FC */

    /* 6. Modbus TCP Parse Response */
    u8 tcp_resp[15] = {
        0x12, 0x34,             /* Tx ID */
        0x00, 0x00,             /* Proto */
        0x00, 0x07,             /* Len = 1 (unit) + 1 (fc) + 1 (byte_count) + 4 (data) = 7 */
        0x01,                   /* Unit ID */
        0x03,                   /* FC */
        0x04,                   /* Byte count = 4 (2 regs) */
        0x03, 0xE8,             /* Reg 0 = 1000 */
        0x07, 0xD0              /* Reg 1 = 2000 */
    };
    parse_ok = ModbusTcpParseRead(tcp_resp, 15, 0x1234, 1, parsed_regs, 4, &parsed_count);
    assert(parse_ok);
    CP_UNUSED(parse_ok);
    assert(parsed_count == 2);
    assert(parsed_regs[0] == 1000);
    assert(parsed_regs[1] == 2000);

    printf("       Modbus RTU/TCP frames built and parsed with 100%% precision.\n");
}

/* -------------------------------------------------------------------------
 * Test 8: CAN Bus Frame Packing & Unpacking
 * ------------------------------------------------------------------------- */
static void test_can_bus(void) {
    printf("[TEST] cp_net: SocketCAN frame packing, unpack, and extended ID validation...\n");

    /* 1. Standard 11-bit Frame */
    cp_can_frame_t frame;
    u8 payload[8] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };
    CanPack(&frame, 0x7DF, payload, 8, false);

    assert(frame.can_id == 0x7DF);
    assert(frame.can_dlc == 8);
    assert(!frame.is_extended);
    assert(memcmp(frame.data, payload, 8) == 0);

    u32 unpack_id = 0;
    u8 unpack_data[8];
    u8 unpack_dlc = 0;
    bool unpack_ext = true;
    bool ok = CanUnpack(&frame, &unpack_id, unpack_data, &unpack_dlc, &unpack_ext);
    assert(ok);
    assert(unpack_id == 0x7DF);
    assert(unpack_dlc == 8);
    assert(!unpack_ext);
    assert(memcmp(unpack_data, payload, 8) == 0);

    /* 2. Extended 29-bit Frame */
    u8 ext_payload[4] = { 0x01, 0x02, 0x03, 0x04 };
    CanPack(&frame, 0x18EAFF01, ext_payload, 4, true);

    ok = CanUnpack(&frame, &unpack_id, unpack_data, &unpack_dlc, &unpack_ext);
    assert(ok);
    CP_UNUSED(ok);
    assert(unpack_id == 0x18EAFF01);
    assert(unpack_dlc == 4);
    assert(unpack_ext);
    assert(memcmp(unpack_data, ext_payload, 4) == 0);

    printf("       CAN 2.0A and 2.0B frame pack/unpack verified with bit-exact parity.\n");
}

/* -------------------------------------------------------------------------
 * Test 9: Dot-Notation Namespaces
 * ------------------------------------------------------------------------- */
static void test_dot_notation(void) {
    printf("[TEST] cp_thread & cp_net: Dot-notation namespaces verification...\n");

    /* thread.yield */
    thread.yield();

    /* mutex namespace */
    cp_mutex_t m;
    mutex.init(&m);
    mutex.lock(&m);
    mutex.unlock(&m);
    assert(mutex.trylock(&m));
    mutex.unlock(&m);
    mutex.destroy(&m);

    /* threadpool namespace */
    cp_threadpool_t *tp = threadpool.create(2, 64);
    assert(tp != NULL);
    threadpool.wait(tp);
    threadpool.destroy(tp);

    /* modbus namespace */
    u8 buf[4] = { 1, 2, 3, 4 };
    u16 c1 = modbus.crc16(buf, 4);
    u16 c2 = ModbusCrc16(buf, 4);
    assert(c1 == c2);
    CP_UNUSED(c1);
    CP_UNUSED(c2);

    /* can namespace */
    cp_can_frame_t f;
    can.pack(&f, 0x100, buf, 4, false);
    assert(f.can_id == 0x100);

    printf("       Dot-notation namespaces operating identically to PascalCase.\n");
}

static void parallel_scale_worker(usize start, usize end, void *arg) {
    int *arr = (int *)arg;
    for (usize i = start; i < end; ++i) {
        arr[i] *= 3;
    }
}

static void test_thread_pool_parallel_for(void) {
    printf("[TEST] cp_thread: ThreadPool ParallelFor & VecParallelMap...\n");

    ThreadPool *pool = ThreadPoolCreate(0, 0);
    assert(pool != NULL);
    assert(ThreadPoolNumThreads(pool) >= 1);
    assert(threadpool.numThreads(pool) >= 1);

    const usize n = 40000;
    int *arr = (int *)malloc(n * sizeof(int));
    assert(arr != NULL);
    for (usize i = 0; i < n; ++i) arr[i] = 1;

    /* Execute ParallelFor with static function */
    ParallelFor(pool, n, parallel_scale_worker, arr);
    for (usize i = 0; i < n; ++i) {
        assert(arr[i] == 3);
    }

    /* Execute dot-notation threadpool.parallelFor */
    threadpool.parallelFor(pool, n, parallel_scale_worker, arr);
    for (usize i = 0; i < n; ++i) {
        assert(arr[i] == 9);
    }
    free(arr);

#if defined(CP_HAS_LAMBDA) && CP_HAS_LAMBDA
    /* Test ParallelFor with Lambda */
    int values[1000];
    for (int i = 0; i < 1000; ++i) values[i] = 5;

    ParallelFor(pool, 1000, Lambda(void, (usize start, usize end, void *arg), {
        int *v = (int *)arg;
        for (usize i = start; i < end; ++i) {
            v[i] += 10;
        }
    }), values);

    for (int i = 0; i < 1000; ++i) {
        assert(values[i] == 15);
    }

    /* Test VecParallelMap */
    Vec(int) vec = VecNew(int);
    for (int i = 0; i < 2000; ++i) {
        VecPush(vec, 2);
    }
    VecParallelMap(pool, vec, {
        *it = *it * 4;
    });
    for (int i = 0; i < 2000; ++i) {
        assert(vec[i] == 8);
    }
    VecFree(vec);
#endif

    ThreadPoolDestroy(pool);
    printf("       ThreadPool ParallelFor successfully parallelized across all cores.\n");
}

int main(void) {
    printf("===============================================================================\n");
    printf("               Cplus Concurrency & Networking Unit Test Suite                  \n");
    printf("===============================================================================\n");

    test_thread_basic();
    test_mutex_and_scoped_lock();
    test_condvar();
    test_thread_pool();
    test_thread_pool_parallel_for();
    test_net_tcp_loopback();
    test_net_udp();
    test_modbus();
    test_can_bus();
    test_dot_notation();

    printf("===============================================================================\n");
    printf("       ALL CONCURRENCY & NETWORKING TESTS PASSED WITH ZERO WARNINGS/ERRORS     \n");
    printf("===============================================================================\n");
    return 0;
}
