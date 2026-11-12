# Module Specification: cp_ds (Collections & Data Structures)

## 1. TL;DR
The `cp_ds` module provides cacheline-aligned, zero-overhead collections: type-safe stretchy buffer dynamic arrays (`Vec(T)`), functional inlined iterators (`VecEach`, `VecMap`), a lock-free Single-Producer Single-Consumer circular queue (`RingBuffer`), and Arena-backed singly/doubly linked lists (`SList`, `DList`).

```c
#include "cplus/cplus.h"

/* Dynamic Type-Safe Vector */
Vec(int) numbers = VecNew(int);
VecPush(numbers, 10);
VecPush(numbers, 20);

/* In-Place Transformation */
VecMap(numbers, it, { *it *= 2; });

/* Direct array access */
printf("Item 0: %d, Len: %zu\n", numbers[0], VecLen(numbers));
VecFree(numbers);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Dynamic Stretchy Vector (`Vec(T)`)
```c
typedef struct {
    usize len;
    usize cap;
} cp_vec_header_t;

#define cp_vec_header(v) ((cp_vec_header_t *)(v) - 1)
#define Vec(T) T*
```
- **Contiguous Layout**: A `Vec(T)` is a standard C pointer to `T`. The hidden header `cp_vec_header_t` resides directly before element 0 in memory.
- **Zero Function Call Overhead**: Indexing (`v[i]`) compiles directly to machine pointer offsets (e.g. `MOV EAX, [RDI + RSI*4]`), bypassing all bounds checking or abstraction layers.
- **Geometric Growth**: When `len == cap`, capacity doubles ($2\times$) using `realloc()`.

### 2.2 Lock-Free SPSC Queue (`RingBuffer`)
```c
typedef struct cp_ringbuf {
    void **buffer;
    usize capacity;
    usize mask;
    _Alignas(64) volatile usize head;
    _Alignas(64) volatile usize tail;
} RingBuffer;
```
- **Cacheline Isolation**: `head` and `tail` are aligned to separate 64-byte hardware cachelines (`_Alignas(64)`), completely eliminating false sharing between producer and consumer CPU cores.
- **Bitwise Wraparound**: Capacity is constrained to powers of two, replacing slow integer division with bitwise AND masking (`mask = capacity - 1`).

---

## 3. Operational Guarantees & Contracts

| Metric | Vec(T) | RingBuffer (SPSC) | SList / DList |
|---|---|---|---|
| **Element Indexing** | $O(1)$ hardware array access | N/A (FIFO) | $O(N)$ traversal |
| **Push / Enqueue** | $O(1)$ amortized | $O(1)$ lock-free atomic | $O(1)$ front/back |
| **Pop / Dequeue** | $O(1)$ | $O(1)$ lock-free atomic | $O(1)$ front |
| **Arbitrary Removal**| $O(N)$ shift | N/A | $O(1)$ for `DList` with node ptr |
| **Thread-Safety** | Not thread-safe | Safe for 1 Producer + 1 Consumer | Thread-confined |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| New Vector | `VecNew(T)` | `vec.new(T)` | Initialize empty vector pointer |
| Push Element | `VecPush(v, val)` | `vec.push(v, val)` | Append item, grow capacity (x2) |
| Pop Element | `VecPop(v)` | `vec.pop(v)` | Decrement length by 1 |
| Vector Length | `VecLen(v)` | `vec.len(v)` | Get current element count |
| Vector Capacity | `VecCap(v)` | `vec.cap(v)` | Get total buffer capacity |
| Free Vector | `VecFree(v)` | `vec.free(v)` | Deallocate buffer, set v = NULL |
| Each Loop | `VecEach(v, idx, val, body)` | `vec.each(...)` | Inlined read loop by value |
| Map Loop | `VecMap(v, it, body)` | `vec.map(...)` | Inlined in-place pointer mutation |
| Create RingBuf | `RingBufCreate(pow2_cap)` | `ringbuf.create(cap)` | Allocate lock-free SPSC queue |
| RingBuf Push | `RingBufPush(rb, item)` | `ringbuf.push(rb, item)` | Non-blocking atomic enqueue |
| RingBuf Pop | `RingBufPop(rb)` | `ringbuf.pop(rb)` | Non-blocking atomic dequeue |
| Destroy RingBuf| `RingBufDestroy(rb)` | `ringbuf.destroy(rb)` | Deallocate ring buffer |

---

## 5. Use Cases
1. **Low-Latency Inter-Thread Messaging**: Pass telemetry data or network packets from an I/O thread to an engine thread using `RingBuffer` without mutex lock contention.
2. **Dynamic In-Place Array Transformations**: Use `VecMap` to scale or filter numerical data in contiguous cache memory.
3. **Graph Adjacency Lists**: Backed by `DList` allocated on an `Arena` for zero-leak graph manipulation.

---

## 6. Complete C11 Usage Examples

### Example 1: Lock-Free Worker Channel
```c
#include "cplus/cplus.h"

RingBuffer *channel;

void *producer_thread(void *arg) {
    for (intptr_t i = 0; i < 1000; i++) {
        while (!RingBufPush(channel, (void *)i)) {
            ThreadYield(); // Buffer full, yield time-slice
        }
    }
    return NULL;
}

void *consumer_thread(void *arg) {
    usize received = 0;
    while (received < 1000) {
        void *data = RingBufPop(channel);
        if (data != NULL) {
            received++;
        } else {
            ThreadYield();
        }
    }
    return NULL;
}
```

---

## 7. Limitations & Operational Constraints
- **Vector Reallocation Invalidation**: Calling `VecPush` may reallocate the vector's memory block. Do not retain direct element pointers (`&v[i]`) across calls to `VecPush`.
- **RingBuffer Capacity**: `RingBufCreate` requires capacity to be an exact power of two (e.g. 16, 64, 1024, 4096).
- **SPSC Only**: `RingBuffer` is strictly Single-Producer Single-Consumer. Multiple producers or multiple consumers concurrently writing to the same ring buffer will cause data races. For multi-producer multi-consumer queues, use `ThreadPool` or `Mutex`.
