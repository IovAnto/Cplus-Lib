# Module Specification: cp_net (Networking & Industrial Fieldbuses)

## 1. TL;DR
The `cp_net` module provides non-blocking TCP and UDP socket wrappers, high-throughput Modbus RTU/TCP industrial framing with hardware-lookup CRC16, and Linux SocketCAN framing.

```c
#include "cplus/cplus.h"

/* 1. Ergonomic TCP Request One-Liner */
char response[512];
usize resp_len = 0;
bool ok = NetTcpRequest("127.0.0.1", 8080, "GET /status", 11, response, sizeof(response), &resp_len, 2000);

/* 2. Modbus CRC16 Calculation */
u8 modbus_frame[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x0A };
u16 crc = ModbusCrc16(modbus_frame, sizeof(modbus_frame));
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Socket Descriptors
```c
typedef int Socket;
#define CP_INVALID_SOCKET (-1)
```
- Standard POSIX file descriptors configured for non-blocking asynchronous event loops (`O_NONBLOCK`).
- Built-in socket options: `SO_REUSEADDR` for instant server restarts without `EADDRINUSE`, and `TCP_NODELAY` to disable Nagle's algorithm for sub-millisecond RPC latency.

### 2.2 Industrial Modbus & CAN Structures
```c
typedef struct {
    u32 can_id;
    u8 len;
    u8 data[8];
} cp_can_frame_t;
```
- Modbus RTU framing with pre-calculated table-driven CRC16 computation (polynomial `0xA001`, initial value `0xFFFF`), processing frames in a single memory pass.

---

## 3. Operational Guarantees & Contracts

| Metric | TCP / UDP Sockets | Modbus CRC16 | SocketCAN |
|---|---|---|---|
| **I/O Latency** | Non-blocking (kernel poll) | Sub-microsecond (table lookup) | Direct kernel socket |
| **Memory Allocation** | 0 heap allocations | 0B (static table) | 0 heap allocations |
| **Timeout Handling** | Millisecond precision (`poll`) | Pure calculation | Millisecond precision |
| **Thread-Safety** | Thread-safe socket descriptors| Pure function | Thread-safe descriptors |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| TCP Connect | `NetTcpConnect(h, port, to_ms)`| `net.tcpConnect(...)` | Connect to remote TCP host with timeout |
| TCP Listen | `NetTcpListen(addr, port, b)`| `net.tcpListen(...)` | Bind and listen for incoming connections |
| TCP Accept | `NetTcpAccept(fd, ip, sz, &p)`| `net.tcpAccept(...)` | Accept client, extract IP and port |
| TCP Request | `NetTcpRequest(h, p, req, ...)`| `net.tcpRequest(...)` | One-liner request/response client |
| Send All | `NetSendAll(fd, buf, len, to)`| `net.sendAll(...)` | Reliably send exact byte stream |
| Receive | `NetRecv(fd, buf, cap, to)` | `net.recv(...)` | Non-blocking read with timeout |
| UDP Bind | `NetUdpBind(addr, port)` | `net.udpBind(...)` | Bind local UDP socket |
| UDP Send To | `NetUdpSendTo(fd, h, p, d, l)`| `net.udpSendTo(...)` | Send UDP datagram to target |
| UDP Send | `NetUdpSend(h, p, d, l)` | `net.udpSend(...)` | One-liner UDP datagram dispatch |
| Close Socket | `NetClose(fd)` | `net.close(fd)` | Graceful socket shutdown and close |
| Modbus CRC16 | `ModbusCrc16(buf, len)` | `modbus.crc16(...)` | 256-word table CRC16 calculation |
| Pack Read Holding | `ModbusRtuPackReadHolding(...)`| `modbus.packReadHolding(...)` | Pack Modbus function code 0x03 |
| CAN Open | `CanSocketOpen(ifname)` | `can.open(...)` | Open Linux SocketCAN interface |
| CAN Send | `CanSend(fd, frame)` | `can.send(...)` | Transmit CAN 2.0A/B frame |

---

## 5. Use Cases
1. **Microservice RPC Clients**: Issue fast non-blocking TCP queries using `NetTcpRequest`.
2. **Industrial Automation & SCADA**: Communicate with PLCs, VFDs, and sensors via Modbus RTU/TCP.
3. **Automotive & Telematics**: Read vehicle sensors via Linux SocketCAN (`vcan0` / `can0`).

---

## 6. Complete C11 Usage Examples

### Example: UDP Telemetry Dispatch
```c
#include "cplus/cplus.h"

void broadcast_telemetry(float temp, float pressure) {
    char payload[64];
    int len = snprintf(payload, sizeof(payload), "{\"temp\":%.2f,\"pres\":%.2f}", temp, pressure);
    
    // One-liner: sends UDP datagram without managing socket lifecycle manually
    NetUdpSend("192.168.1.255", 9000, payload, (usize)len);
}
```

---

## 7. Limitations & Operational Constraints
- **Platform Support for CAN**: `CanSocketOpen` requires a Linux kernel with `AF_CAN` support enabled. On macOS and Windows, CAN APIs return error codes.
- **TCP Request Buffering**: `NetTcpRequest` reads up to `resp_buf_cap` bytes. If the server response exceeds the buffer capacity, data is truncated to prevent buffer overflows.
