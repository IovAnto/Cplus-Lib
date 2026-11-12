/**
 * @file cp_net.c
 * @brief High-performance networking primitives, non-blocking TCP/UDP, Modbus RTU/TCP, and SocketCAN.
 */

#include "cplus/cp_net.h"

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
