/**
 * @file cp_net.h
 * @brief High-performance networking primitives, non-blocking TCP/UDP, and Modbus/CAN industrial protocols.
 */

#ifndef CPLUS_NET_H
#define CPLUS_NET_H

#include "cp_core.h"

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

#endif /* CPLUS_NET_H */
