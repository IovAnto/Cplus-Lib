/**
 * @file cp_net.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_net.
 *
 * In ONE file define:
 *   #define CPLUS_CP_NET_IMPLEMENTATION
 *   #include "cp_net.h"
 */
#ifndef CPLUS_CP_NET_H
#define CPLUS_CP_NET_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
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
 * @file cp_net.h
 * @brief High-performance networking primitives, non-blocking TCP/UDP, and Modbus/CAN industrial protocols.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Socket Descriptors & Network Configuration
 * ------------------------------------------------------------------------- */
typedef int cp_socket_t;
typedef cp_socket_t Socket;
#define CP_INVALID_SOCKET (-1)

void cp_net_close(cp_socket_t fd);
bool cp_net_set_nonblocking(cp_socket_t fd, bool nonblocking);
bool cp_net_set_reuseaddr(cp_socket_t fd, bool reuse);
bool cp_net_set_tcp_nodelay(cp_socket_t fd, bool nodelay);

/* -------------------------------------------------------------------------
 * TCP Sockets
 * ------------------------------------------------------------------------- */
cp_socket_t cp_net_tcp_connect(const char *host, u16 port, u32 timeout_ms);
cp_socket_t cp_net_tcp_listen(const char *bind_addr, u16 port, i32 backlog);
cp_socket_t cp_net_tcp_accept(cp_socket_t listen_fd, char *client_ip_out, usize ip_len, u16 *client_port_out);

i64 cp_net_send_all(cp_socket_t fd, const void *buf, usize len, u32 timeout_ms);
i64 cp_net_recv(cp_socket_t fd, void *buf, usize cap, u32 timeout_ms);

/**
 * @brief Ergonomic one-liner: connect, send request, receive full response, and close.
 */
bool cp_net_tcp_request(const char *host, u16 port, const void *req, usize req_len,
                        void *resp_buf, usize resp_buf_cap, usize *resp_len_out, u32 timeout_ms);

/* -------------------------------------------------------------------------
 * UDP Sockets
 * ------------------------------------------------------------------------- */
cp_socket_t cp_net_udp_bind(const char *bind_addr, u16 port);
i64 cp_net_udp_send_to(cp_socket_t fd, const char *host, u16 port, const void *data, usize len);
i64 cp_net_udp_recv_from(cp_socket_t fd, void *buf, usize cap, char *sender_ip, usize ip_len, u16 *sender_port, u32 timeout_ms);

/**
 * @brief Ergonomic one-liner: send UDP packet to target without manual socket lifecycle.
 */
bool cp_net_udp_send(const char *host, u16 port, const void *data, usize len);

/* -------------------------------------------------------------------------
 * Industrial Protocols: Modbus (RTU & TCP)
 * ------------------------------------------------------------------------- */
/**
 * @brief Hardware-lookup fast Modbus CRC16 (poly 0xA001, init 0xFFFF).
 */
u16 cp_modbus_crc16(const u8 *buf, usize len);

/* Modbus RTU framing */
usize cp_modbus_rtu_build_read_holding_registers(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count);
usize cp_modbus_rtu_build_write_single_register(u8 *out, usize cap, u8 slave_id, u16 reg_addr, u16 value);
usize cp_modbus_rtu_build_write_multiple_registers(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count, const u16 *values);
bool cp_modbus_rtu_verify_crc(const u8 *frame, usize len);
bool cp_modbus_rtu_parse_read_response(const u8 *frame, usize len, u8 expected_slave, u16 *out_regs, u16 max_regs, u16 *out_reg_count);

/* Modbus TCP framing (MBAP Header + PDU) */
usize cp_modbus_tcp_build_read_holding_registers(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count);
usize cp_modbus_tcp_build_write_multiple_registers(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count, const u16 *values);
bool cp_modbus_tcp_parse_read_response(const u8 *frame, usize len, u16 expected_tx_id, u8 expected_unit_id, u16 *out_regs, u16 max_regs, u16 *out_reg_count);

/* -------------------------------------------------------------------------
 * Industrial Protocols: CAN Bus (SocketCAN & Frame Packing)
 * ------------------------------------------------------------------------- */
typedef struct {
    u32 can_id;       /**< 11-bit standard or 29-bit extended ID */
    u8  can_dlc;      /**< Length code: 0..8 bytes */
    u8  data[8];      /**< Payload */
    bool is_extended; /**< True for 29-bit extended frame */
    bool is_rtr;      /**< True for Remote Transmission Request */
} cp_can_frame_t;

/* Portable frame pack/unpack */
void cp_can_pack(cp_can_frame_t *frame, u32 id, const void *data, u8 len, bool is_extended);
bool cp_can_unpack(const cp_can_frame_t *frame, u32 *out_id, void *out_data, u8 *out_len, bool *out_is_extended);

/* Linux SocketCAN native network interface */
cp_socket_t cp_can_open(const char *iface_name);
bool cp_can_send(cp_socket_t fd, const cp_can_frame_t *frame);
bool cp_can_recv(cp_socket_t fd, cp_can_frame_t *out_frame, u32 timeout_ms);
bool cp_can_set_filter(cp_socket_t fd, u32 can_id, u32 can_mask);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define NetClose(fd)                         cp_net_close(fd)
#define NetSetNonblocking(fd, nb)            cp_net_set_nonblocking(fd, nb)
#define NetSetReuseAddr(fd, r)               cp_net_set_reuseaddr(fd, r)
#define NetSetTcpNoDelay(fd, nd)             cp_net_set_tcp_nodelay(fd, nd)

#define NetTcpConnect(h, p, to)              cp_net_tcp_connect(h, p, to)
#define NetTcpListen(addr, p, bl)            cp_net_tcp_listen(addr, p, bl)
#define NetTcpAccept(lfd, ip, ipl, port)     cp_net_tcp_accept(lfd, ip, ipl, port)
#define NetSendAll(fd, buf, len, to)         cp_net_send_all(fd, buf, len, to)
#define NetRecv(fd, buf, cap, to)            cp_net_recv(fd, buf, cap, to)
#define NetTcpRequest(h, p, r, rl, rb, rbc, ro, to) cp_net_tcp_request(h, p, r, rl, rb, rbc, ro, to)

#define NetUdpBind(addr, p)                  cp_net_udp_bind(addr, p)
#define NetUdpSendTo(fd, h, p, d, l)         cp_net_udp_send_to(fd, h, p, d, l)
#define NetUdpRecvFrom(fd, b, c, ip, il, p, to) cp_net_udp_recv_from(fd, b, c, ip, il, p, to)
#define NetUdpSend(h, p, d, l)               cp_net_udp_send(h, p, d, l)

#define ModbusCrc16(b, l)                    cp_modbus_crc16(b, l)
#define ModbusRtuBuildRead(o, c, s, a, n)    cp_modbus_rtu_build_read_holding_registers(o, c, s, a, n)
#define ModbusRtuBuildWriteSingle(o, c, s, a, v) cp_modbus_rtu_build_write_single_register(o, c, s, a, v)
#define ModbusRtuBuildWriteMultiple(o, c, s, a, n, v) cp_modbus_rtu_build_write_multiple_registers(o, c, s, a, n, v)
#define ModbusRtuVerifyCrc(f, l)             cp_modbus_rtu_verify_crc(f, l)
#define ModbusRtuParseRead(f, l, s, o, m, c) cp_modbus_rtu_parse_read_response(f, l, s, o, m, c)

#define ModbusTcpBuildRead(o, c, tx, u, a, n) cp_modbus_tcp_build_read_holding_registers(o, c, tx, u, a, n)
#define ModbusTcpBuildWriteMultiple(o, c, tx, u, a, n, v) cp_modbus_tcp_build_write_multiple_registers(o, c, tx, u, a, n, v)
#define ModbusTcpParseRead(f, l, tx, u, o, m, c) cp_modbus_tcp_parse_read_response(f, l, tx, u, o, m, c)

#define CanPack(f, id, d, l, ext)            cp_can_pack(f, id, d, l, ext)
#define CanUnpack(f, id, d, l, ext)          cp_can_unpack(f, id, d, l, ext)
#define CanOpen(iface)                       cp_can_open(iface)
#define CanSend(fd, f)                       cp_can_send(fd, f)
#define CanRecv(fd, f, to)                   cp_can_recv(fd, f, to)
#define CanSetFilter(fd, id, mask)           cp_can_set_filter(fd, id, mask)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespaces
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*const close)(cp_socket_t fd);
    bool (*const setNonblocking)(cp_socket_t fd, bool nonblocking);
    bool (*const setReuseAddr)(cp_socket_t fd, bool reuse);
    bool (*const setTcpNoDelay)(cp_socket_t fd, bool nodelay);
    cp_socket_t (*const tcpConnect)(const char *host, u16 port, u32 timeout_ms);
    cp_socket_t (*const tcpListen)(const char *bind_addr, u16 port, i32 backlog);
    cp_socket_t (*const tcpAccept)(cp_socket_t listen_fd, char *client_ip_out, usize ip_len, u16 *client_port_out);
    i64 (*const sendAll)(cp_socket_t fd, const void *buf, usize len, u32 timeout_ms);
    i64 (*const recv)(cp_socket_t fd, void *buf, usize cap, u32 timeout_ms);
    bool (*const tcpRequest)(const char *host, u16 port, const void *req, usize req_len,
                             void *resp_buf, usize resp_buf_cap, usize *resp_len_out, u32 timeout_ms);
    cp_socket_t (*const udpBind)(const char *bind_addr, u16 port);
    i64 (*const udpSendTo)(cp_socket_t fd, const char *host, u16 port, const void *data, usize len);
    i64 (*const udpRecvFrom)(cp_socket_t fd, void *buf, usize cap, char *sender_ip, usize ip_len, u16 *sender_port, u32 timeout_ms);
    bool (*const udpSend)(const char *host, u16 port, const void *data, usize len);
} cp_net_namespace_t;

typedef struct {
    u16 (*const crc16)(const u8 *buf, usize len);
    usize (*const rtuBuildRead)(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count);
    usize (*const rtuBuildWriteSingle)(u8 *out, usize cap, u8 slave_id, u16 reg_addr, u16 value);
    usize (*const rtuBuildWriteMultiple)(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count, const u16 *values);
    bool (*const rtuVerifyCrc)(const u8 *frame, usize len);
    bool (*const rtuParseRead)(const u8 *frame, usize len, u8 expected_slave, u16 *out_regs, u16 max_regs, u16 *out_reg_count);
    usize (*const tcpBuildRead)(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count);
    usize (*const tcpBuildWriteMultiple)(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count, const u16 *values);
    bool (*const tcpParseRead)(const u8 *frame, usize len, u16 expected_tx_id, u8 expected_unit_id, u16 *out_regs, u16 max_regs, u16 *out_reg_count);
} cp_modbus_namespace_t;

typedef struct {
    void (*const pack)(cp_can_frame_t *frame, u32 id, const void *data, u8 len, bool is_extended);
    bool (*const unpack)(const cp_can_frame_t *frame, u32 *out_id, void *out_data, u8 *out_len, bool *out_is_extended);
    cp_socket_t (*const open)(const char *iface_name);
    bool (*const send)(cp_socket_t fd, const cp_can_frame_t *frame);
    bool (*const recv)(cp_socket_t fd, cp_can_frame_t *out_frame, u32 timeout_ms);
    bool (*const setFilter)(cp_socket_t fd, u32 can_id, u32 can_mask);
} cp_can_namespace_t;

static const cp_net_namespace_t net = {
    .close = cp_net_close,
    .setNonblocking = cp_net_set_nonblocking,
    .setReuseAddr = cp_net_set_reuseaddr,
    .setTcpNoDelay = cp_net_set_tcp_nodelay,
    .tcpConnect = cp_net_tcp_connect,
    .tcpListen = cp_net_tcp_listen,
    .tcpAccept = cp_net_tcp_accept,
    .sendAll = cp_net_send_all,
    .recv = cp_net_recv,
    .tcpRequest = cp_net_tcp_request,
    .udpBind = cp_net_udp_bind,
    .udpSendTo = cp_net_udp_send_to,
    .udpRecvFrom = cp_net_udp_recv_from,
    .udpSend = cp_net_udp_send,
};

static const cp_modbus_namespace_t modbus = {
    .crc16 = cp_modbus_crc16,
    .rtuBuildRead = cp_modbus_rtu_build_read_holding_registers,
    .rtuBuildWriteSingle = cp_modbus_rtu_build_write_single_register,
    .rtuBuildWriteMultiple = cp_modbus_rtu_build_write_multiple_registers,
    .rtuVerifyCrc = cp_modbus_rtu_verify_crc,
    .rtuParseRead = cp_modbus_rtu_parse_read_response,
    .tcpBuildRead = cp_modbus_tcp_build_read_holding_registers,
    .tcpBuildWriteMultiple = cp_modbus_tcp_build_write_multiple_registers,
    .tcpParseRead = cp_modbus_tcp_parse_read_response,
};

static const cp_can_namespace_t can = {
    .pack = cp_can_pack,
    .unpack = cp_can_unpack,
    .open = cp_can_open,
    .send = cp_can_send,
    .recv = cp_can_recv,
    .setFilter = cp_can_set_filter,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_NET_H */

#ifdef CPLUS_CP_NET_IMPLEMENTATION
/**
 * @file cp_net.c
 * @brief High-performance networking primitives, non-blocking TCP/UDP, Modbus RTU/TCP, and SocketCAN.
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

/* -------------------------------------------------------------------------
 * Socket Descriptors & Configuration
 * ------------------------------------------------------------------------- */
void cp_net_close(cp_socket_t fd) {
    if (fd >= 0) {
        close(fd);
    }
}

bool cp_net_set_nonblocking(cp_socket_t fd, bool nonblocking) {
    if (fd < 0) return false;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    if (nonblocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    return fcntl(fd, F_SETFL, flags) != -1;
}

bool cp_net_set_reuseaddr(cp_socket_t fd, bool reuse) {
    if (fd < 0) return false;
    int opt = reuse ? 1 : 0;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}

bool cp_net_set_tcp_nodelay(cp_socket_t fd, bool nodelay) {
    if (fd < 0) return false;
    int opt = nodelay ? 1 : 0;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == 0;
}

/* -------------------------------------------------------------------------
 * TCP Sockets
 * ------------------------------------------------------------------------- */
cp_socket_t cp_net_tcp_connect(const char *host, u16 port, u32 timeout_ms) {
    if (!host) return CP_INVALID_SOCKET;
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    struct addrinfo hints, *res = NULL, *p = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* IPv4 */
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        return CP_INVALID_SOCKET;
    }

    cp_socket_t fd = CP_INVALID_SOCKET;
    for (p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;

        cp_net_set_tcp_nodelay(fd, true);

        if (timeout_ms > 0) {
            cp_net_set_nonblocking(fd, true);
            int ret = connect(fd, p->ai_addr, p->ai_addrlen);
            if (ret == 0) {
                /* Connected immediately */
                cp_net_set_nonblocking(fd, false);
                break;
            }
            if (errno == EINPROGRESS) {
                struct pollfd pfd = { .fd = fd, .events = POLLOUT, .revents = 0 };
                int pr = poll(&pfd, 1, (int)timeout_ms);
                if (pr > 0 && (pfd.revents & POLLOUT)) {
                    int err = 0;
                    socklen_t elen = sizeof(err);
                    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen) == 0 && err == 0) {
                        cp_net_set_nonblocking(fd, false);
                        break; /* Success */
                    }
                }
            }
            close(fd);
            fd = CP_INVALID_SOCKET;
        } else {
            if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
                break; /* Success */
            }
            close(fd);
            fd = CP_INVALID_SOCKET;
        }
    }

    freeaddrinfo(res);
    return fd;
}

cp_socket_t cp_net_tcp_listen(const char *bind_addr, u16 port, i32 backlog) {
    cp_socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return CP_INVALID_SOCKET;

    cp_net_set_reuseaddr(fd, true);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (!bind_addr || bind_addr[0] == '\0' || strcmp(bind_addr, "0.0.0.0") == 0) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, bind_addr, &sin.sin_addr) <= 0) {
            close(fd);
            return CP_INVALID_SOCKET;
        }
    }

    if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }

    if (listen(fd, (backlog > 0) ? backlog : 128) != 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }

    return fd;
}

cp_socket_t cp_net_tcp_accept(cp_socket_t listen_fd, char *client_ip_out, usize ip_len, u16 *client_port_out) {
    if (listen_fd < 0) return CP_INVALID_SOCKET;
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    cp_socket_t client = accept(listen_fd, (struct sockaddr *)&sin, &slen);
    if (client < 0) return CP_INVALID_SOCKET;

    cp_net_set_tcp_nodelay(client, true);

    if (client_ip_out && ip_len > 0) {
        inet_ntop(AF_INET, &sin.sin_addr, client_ip_out, (socklen_t)ip_len);
    }
    if (client_port_out) {
        *client_port_out = ntohs(sin.sin_port);
    }
    return client;
}

i64 cp_net_send_all(cp_socket_t fd, const void *buf, usize len, u32 timeout_ms) {
    if (fd < 0 || !buf) return -1;
    usize sent = 0;
    const u8 *ptr = (const u8 *)buf;
    while (sent < len) {
        if (timeout_ms > 0) {
            struct pollfd pfd = { .fd = fd, .events = POLLOUT, .revents = 0 };
            int r = poll(&pfd, 1, (int)timeout_ms);
            if (r <= 0) return (sent > 0) ? (i64)sent : -1;
        }
        ssize_t n = send(fd, ptr + sent, len - sent, MSG_NOSIGNAL);
        if (n > 0) {
            sent += (usize)n;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            struct pollfd pfd = { .fd = fd, .events = POLLOUT, .revents = 0 };
            int r = poll(&pfd, 1, (timeout_ms > 0) ? (int)timeout_ms : 5000);
            if (r <= 0) return (sent > 0) ? (i64)sent : -1;
        } else {
            return (sent > 0) ? (i64)sent : -1;
        }
    }
    return (i64)sent;
}

i64 cp_net_recv(cp_socket_t fd, void *buf, usize cap, u32 timeout_ms) {
    if (fd < 0 || !buf || cap == 0) return -1;
    if (timeout_ms > 0) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
        int r = poll(&pfd, 1, (int)timeout_ms);
        if (r <= 0) return -1;
    }
    ssize_t n = recv(fd, buf, cap, 0);
    return (i64)n;
}

bool cp_net_tcp_request(const char *host, u16 port, const void *req, usize req_len,
                        void *resp_buf, usize resp_buf_cap, usize *resp_len_out, u32 timeout_ms) {
    cp_socket_t fd = cp_net_tcp_connect(host, port, timeout_ms);
    if (fd < 0) return false;

    if (req && req_len > 0) {
        if (cp_net_send_all(fd, req, req_len, timeout_ms) < (i64)req_len) {
            cp_net_close(fd);
            return false;
        }
    }

    usize total_recv = 0;
    if (resp_buf && resp_buf_cap > 0) {
        i64 n = cp_net_recv(fd, resp_buf, resp_buf_cap, timeout_ms);
        if (n > 0) {
            total_recv = (usize)n;
        }
    }
    if (resp_len_out) {
        *resp_len_out = total_recv;
    }
    cp_net_close(fd);
    return true;
}

/* -------------------------------------------------------------------------
 * UDP Sockets
 * ------------------------------------------------------------------------- */
cp_socket_t cp_net_udp_bind(const char *bind_addr, u16 port) {
    cp_socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return CP_INVALID_SOCKET;
    cp_net_set_reuseaddr(fd, true);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (!bind_addr || bind_addr[0] == '\0' || strcmp(bind_addr, "0.0.0.0") == 0) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, bind_addr, &sin.sin_addr) <= 0) {
            close(fd);
            return CP_INVALID_SOCKET;
        }
    }

    if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }
    return fd;
}

i64 cp_net_udp_send_to(cp_socket_t fd, const char *host, u16 port, const void *data, usize len) {
    if (fd < 0 || !host || !data) return -1;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &sin.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(host);
        if (!he) return -1;
        memcpy(&sin.sin_addr, he->h_addr_list[0], sizeof(sin.sin_addr));
    }
    ssize_t n = sendto(fd, data, len, 0, (struct sockaddr *)&sin, sizeof(sin));
    return (i64)n;
}

i64 cp_net_udp_recv_from(cp_socket_t fd, void *buf, usize cap, char *sender_ip, usize ip_len, u16 *sender_port, u32 timeout_ms) {
    if (fd < 0 || !buf || cap == 0) return -1;
    if (timeout_ms > 0) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
        int r = poll(&pfd, 1, (int)timeout_ms);
        if (r <= 0) return -1;
    }
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    ssize_t n = recvfrom(fd, buf, cap, 0, (struct sockaddr *)&sin, &slen);
    if (n >= 0) {
        if (sender_ip && ip_len > 0) {
            inet_ntop(AF_INET, &sin.sin_addr, sender_ip, (socklen_t)ip_len);
        }
        if (sender_port) {
            *sender_port = ntohs(sin.sin_port);
        }
    }
    return (i64)n;
}

bool cp_net_udp_send(const char *host, u16 port, const void *data, usize len) {
    cp_socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;
    i64 n = cp_net_udp_send_to(fd, host, port, data, len);
    close(fd);
    return n == (i64)len;
}

/* -------------------------------------------------------------------------
 * Industrial Protocols: Modbus
 * ------------------------------------------------------------------------- */
static const u16 cp_modbus_crc_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

u16 cp_modbus_crc16(const u8 *buf, usize len) {
    if (!buf || len == 0) return 0xFFFF;
    u16 crc = 0xFFFF;
    for (usize i = 0; i < len; ++i) {
        u8 idx = (u8)(crc ^ buf[i]);
        crc = (crc >> 8) ^ cp_modbus_crc_table[idx];
    }
    return crc;
}

usize cp_modbus_rtu_build_read_holding_registers(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count) {
    if (!out || cap < 8) return 0;
    out[0] = slave_id;
    out[1] = 0x03; /* Function code: Read Holding Registers */
    out[2] = (u8)(start_addr >> 8);
    out[3] = (u8)(start_addr & 0xFF);
    out[4] = (u8)(count >> 8);
    out[5] = (u8)(count & 0xFF);
    u16 crc = cp_modbus_crc16(out, 6);
    out[6] = (u8)(crc & 0xFF);        /* LSB first */
    out[7] = (u8)((crc >> 8) & 0xFF); /* MSB */
    return 8;
}

usize cp_modbus_rtu_build_write_single_register(u8 *out, usize cap, u8 slave_id, u16 reg_addr, u16 value) {
    if (!out || cap < 8) return 0;
    out[0] = slave_id;
    out[1] = 0x06; /* Function code: Write Single Register */
    out[2] = (u8)(reg_addr >> 8);
    out[3] = (u8)(reg_addr & 0xFF);
    out[4] = (u8)(value >> 8);
    out[5] = (u8)(value & 0xFF);
    u16 crc = cp_modbus_crc16(out, 6);
    out[6] = (u8)(crc & 0xFF);
    out[7] = (u8)((crc >> 8) & 0xFF);
    return 8;
}

usize cp_modbus_rtu_build_write_multiple_registers(u8 *out, usize cap, u8 slave_id, u16 start_addr, u16 count, const u16 *values) {
    usize required = 9 + (usize)count * 2;
    if (!out || cap < required || !values) return 0;

    out[0] = slave_id;
    out[1] = 0x10; /* Function code: Write Multiple Registers */
    out[2] = (u8)(start_addr >> 8);
    out[3] = (u8)(start_addr & 0xFF);
    out[4] = (u8)(count >> 8);
    out[5] = (u8)(count & 0xFF);
    out[6] = (u8)(count * 2); /* Byte count */

    for (usize i = 0; i < count; ++i) {
        out[7 + i * 2] = (u8)(values[i] >> 8);
        out[8 + i * 2] = (u8)(values[i] & 0xFF);
    }

    usize pdu_len = 7 + (usize)count * 2;
    u16 crc = cp_modbus_crc16(out, pdu_len);
    out[pdu_len]     = (u8)(crc & 0xFF);
    out[pdu_len + 1] = (u8)((crc >> 8) & 0xFF);
    return required;
}

bool cp_modbus_rtu_verify_crc(const u8 *frame, usize len) {
    if (!frame || len < 4) return false;
    u16 calculated = cp_modbus_crc16(frame, len - 2);
    u16 received = (u16)frame[len - 2] | ((u16)frame[len - 1] << 8);
    return calculated == received;
}

bool cp_modbus_rtu_parse_read_response(const u8 *frame, usize len, u8 expected_slave, u16 *out_regs, u16 max_regs, u16 *out_reg_count) {
    if (!frame || len < 5 || !cp_modbus_rtu_verify_crc(frame, len)) return false;
    if (expected_slave != 0 && frame[0] != expected_slave) return false;
    if (frame[1] != 0x03) return false;

    u8 byte_count = frame[2];
    if (len != (usize)(3 + byte_count + 2)) return false;

    u16 total_regs = (u16)(byte_count / 2);
    if (out_reg_count) *out_reg_count = total_regs;

    if (out_regs && max_regs > 0) {
        u16 copy_regs = (total_regs < max_regs) ? total_regs : max_regs;
        for (u16 i = 0; i < copy_regs; ++i) {
            out_regs[i] = ((u16)frame[3 + i * 2] << 8) | (u16)frame[4 + i * 2];
        }
    }
    return true;
}

usize cp_modbus_tcp_build_read_holding_registers(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count) {
    if (!out || cap < 12) return 0;
    /* MBAP Header (7 bytes) */
    out[0] = (u8)(tx_id >> 8);
    out[1] = (u8)(tx_id & 0xFF);
    out[2] = 0x00; /* Protocol ID (0 = Modbus) */
    out[3] = 0x00;
    out[4] = 0x00; /* Length: 6 bytes to follow */
    out[5] = 0x06;
    out[6] = unit_id;
    /* PDU */
    out[7] = 0x03; /* Function code: Read Holding Registers */
    out[8] = (u8)(start_addr >> 8);
    out[9] = (u8)(start_addr & 0xFF);
    out[10] = (u8)(count >> 8);
    out[11] = (u8)(count & 0xFF);
    return 12;
}

usize cp_modbus_tcp_build_write_multiple_registers(u8 *out, usize cap, u16 tx_id, u8 unit_id, u16 start_addr, u16 count, const u16 *values) {
    usize required = 13 + (usize)count * 2;
    if (!out || cap < required || !values) return 0;

    u16 remaining_len = (u16)(7 + (usize)count * 2);

    /* MBAP Header */
    out[0] = (u8)(tx_id >> 8);
    out[1] = (u8)(tx_id & 0xFF);
    out[2] = 0x00;
    out[3] = 0x00;
    out[4] = (u8)(remaining_len >> 8);
    out[5] = (u8)(remaining_len & 0xFF);
    out[6] = unit_id;

    /* PDU */
    out[7] = 0x10; /* Function code: Write Multiple Registers */
    out[8] = (u8)(start_addr >> 8);
    out[9] = (u8)(start_addr & 0xFF);
    out[10] = (u8)(count >> 8);
    out[11] = (u8)(count & 0xFF);
    out[12] = (u8)(count * 2);

    for (usize i = 0; i < count; ++i) {
        out[13 + i * 2] = (u8)(values[i] >> 8);
        out[14 + i * 2] = (u8)(values[i] & 0xFF);
    }
    return required;
}

bool cp_modbus_tcp_parse_read_response(const u8 *frame, usize len, u16 expected_tx_id, u8 expected_unit_id, u16 *out_regs, u16 max_regs, u16 *out_reg_count) {
    if (!frame || len < 9) return false;
    u16 tx_id = ((u16)frame[0] << 8) | (u16)frame[1];
    u16 proto = ((u16)frame[2] << 8) | (u16)frame[3];
    u16 length = ((u16)frame[4] << 8) | (u16)frame[5];
    u8 unit_id = frame[6];

    if (expected_tx_id != 0 && tx_id != expected_tx_id) return false;
    if (proto != 0) return false;
    if (expected_unit_id != 0 && unit_id != expected_unit_id) return false;
    if (len < (usize)(6 + length)) return false;

    u8 fc = frame[7];
    if (fc != 0x03) return false;

    u8 byte_count = frame[8];
    u16 total_regs = (u16)(byte_count / 2);
    if (out_reg_count) *out_reg_count = total_regs;

    if (out_regs && max_regs > 0) {
        u16 copy_regs = (total_regs < max_regs) ? total_regs : max_regs;
        for (u16 i = 0; i < copy_regs; ++i) {
            out_regs[i] = ((u16)frame[9 + i * 2] << 8) | (u16)frame[10 + i * 2];
        }
    }
    return true;
}

/* -------------------------------------------------------------------------
 * Industrial Protocols: CAN Bus
 * ------------------------------------------------------------------------- */
void cp_can_pack(cp_can_frame_t *frame, u32 id, const void *data, u8 len, bool is_extended) {
    if (!frame) return;
    frame->can_id = id;
    frame->can_dlc = (len > 8) ? 8 : len;
    frame->is_extended = is_extended;
    frame->is_rtr = false;
    memset(frame->data, 0, 8);
    if (data && frame->can_dlc > 0) {
        memcpy(frame->data, data, frame->can_dlc);
    }
}

bool cp_can_unpack(const cp_can_frame_t *frame, u32 *out_id, void *out_data, u8 *out_len, bool *out_is_extended) {
    if (!frame) return false;
    if (out_id) *out_id = frame->can_id;
    if (out_len) *out_len = frame->can_dlc;
    if (out_is_extended) *out_is_extended = frame->is_extended;
    if (out_data && frame->can_dlc > 0) {
        memcpy(out_data, frame->data, frame->can_dlc);
    }
    return true;
}

cp_socket_t cp_can_open(const char *iface_name) {
    if (!iface_name) return CP_INVALID_SOCKET;
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) return CP_INVALID_SOCKET;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface_name, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return CP_INVALID_SOCKET;
    }
    return fd;
}

bool cp_can_send(cp_socket_t fd, const cp_can_frame_t *frame) {
    if (fd < 0 || !frame) return false;
    struct can_frame raw;
    memset(&raw, 0, sizeof(raw));
    raw.can_id = frame->can_id;
    if (frame->is_extended) raw.can_id |= CAN_EFF_FLAG;
    if (frame->is_rtr) raw.can_id |= CAN_RTR_FLAG;
    raw.can_dlc = frame->can_dlc;
    memcpy(raw.data, frame->data, frame->can_dlc);

    return write(fd, &raw, sizeof(raw)) == (ssize_t)sizeof(raw);
}

bool cp_can_recv(cp_socket_t fd, cp_can_frame_t *out_frame, u32 timeout_ms) {
    if (fd < 0 || !out_frame) return false;
    if (timeout_ms > 0) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
        int r = poll(&pfd, 1, (int)timeout_ms);
        if (r <= 0) return false;
    }
    struct can_frame raw;
    ssize_t n = read(fd, &raw, sizeof(raw));
    if (n != (ssize_t)sizeof(raw)) return false;

    out_frame->is_extended = (raw.can_id & CAN_EFF_FLAG) != 0;
    out_frame->is_rtr = (raw.can_id & CAN_RTR_FLAG) != 0;
    out_frame->can_id = raw.can_id & (out_frame->is_extended ? CAN_EFF_MASK : CAN_SFF_MASK);
    out_frame->can_dlc = raw.can_dlc;
    memcpy(out_frame->data, raw.data, raw.can_dlc);
    return true;
}

bool cp_can_set_filter(cp_socket_t fd, u32 can_id, u32 can_mask) {
    if (fd < 0) return false;
    struct can_filter rfilter;
    rfilter.can_id = can_id;
    rfilter.can_mask = can_mask;
    return setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) == 0;
}
#endif /* CPLUS_CP_NET_IMPLEMENTATION */
