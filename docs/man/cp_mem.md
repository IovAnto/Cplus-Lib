# Module Specification: cp_mem (Memory Subsystem)

## 1. TL;DR
The `cp_mem` subsystem provides high-performance, deterministic memory allocation models designed to eliminate heap fragmentation and allocator lock contention in systems software.

```c
#include "cplus/cplus.h"

/* 1. Linear Arena Allocator */
Arena *arena = ArenaCreate(64 * 1024);
int *buffer = (int *)ArenaAlloc(arena, 1024 * sizeof(int));
ArenaDestroy(arena); // 1 single bulk free

/* 2. Scoped Thread-Local Scratch Arena */
TempArena scratch = ScratchBegin();
char *msg = StrFormat(scratch.arena, "Process PID: %d", OsPid());
ScratchEnd(scratch); // Instant rollback in 1 CPU cycle
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Struct Layout: `Arena` (`cp_arena_t`)
```c
typedef struct cp_arena_chunk {
    struct cp_arena_chunk *prev;
    usize capacity;
    usize offset;
    u8 data[];
} cp_arena_chunk_t;

typedef struct cp_arena {
    usize default_chunk_size;
    cp_arena_chunk_t *current_chunk;
    usize total_allocated;
} Arena;
```
- **Alignment**: Every bump allocation is aligned by default to `CP_DEFAULT_ALIGNMENT` (16 bytes on x86-64), ensuring hardware compliance for all scalar types. Arbitrary alignments (e.g. 32-byte AVX or 64-byte cacheline) are supported via `ArenaAllocAlign`.
- **Chunk Chaining**: If an allocation exceeds the remaining capacity of `current_chunk`, a new chunk of size `max(default_chunk_size, requested_size)` is allocated and chained.
- **Bulk Free**: `ArenaReset` restores the offset of all chunks to 0 without releasing memory to the OS, keeping the virtual address space warm for subsequent frames. `ArenaDestroy` walks the chunk list and releases all memory back to the system.

### 2.2 Struct Layout: `TempArena` (`cp_temp_arena_t`)
```c
typedef struct {
    Arena *arena;
    cp_arena_chunk_t *chunk;
    usize offset;
} TempArena;
```
- A lightweight checkpoint (24 bytes) that captures the exact position of an arena's bump cursor. Calling `ScratchEnd(temp)` restores the pointer in a single CPU register assignment.

### 2.3 Struct Layout: `Pool` (`cp_pool_t`)
```c
typedef struct cp_pool {
    usize item_size;
    usize item_count;
    void *free_list;
    usize used;
} Pool;
```
- A fixed-size slab allocator with an embedded intrusive singly linked free-list. Provides true $O(1)$ allocation and arbitrary $O(1)$ deallocation without heap fragmentation.

---

## 3. Operational Guarantees & Contracts

| Metric | Arena | Scratch Pool | Slab Pool |
|---|---|---|---|
| **Allocation Latency** | $O(1)$ amortized (pointer bump) | $O(1)$ (1 clock cycle) | $O(1)$ (free-list pop) |
| **Deallocation Latency** | $O(1)$ bulk | $O(1)$ rollback | $O(1)$ free-list push |
| **Heap Allocations** | 0 per object (chunked) | 0 (pre-warmed ring) | 0 during runtime |
| **Memory Fragmentation** | 0% (strictly contiguous) | 0% (strictly contiguous) | 0% (fixed slot size) |
| **Thread-Safety** | Thread-confined | Isolated per-thread ring | Requires external lock |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Create Arena | `ArenaCreate(chunk_size)` | `arena.create(chunk_size)` | Instantiate new linear arena |
| Allocate | `ArenaAlloc(arena, size)` | `arena.alloc(arena, size)` | 16-byte aligned bump allocation |
| Allocate Zeroed | `ArenaAllocZero(arena, size)` | `arena.allocZero(arena, size)` | Zero-initialized bump allocation |
| Allocate Aligned | `ArenaAllocAlign(a, sz, align)` | `arena.allocAlign(a, sz, align)` | Arbitrary power-of-two aligned alloc |
| Reset Arena | `ArenaReset(arena)` | `arena.reset(arena)` | Reset cursors without freeing chunks |
| Destroy Arena | `ArenaDestroy(arena)` | `arena.destroy(arena)` | Release all memory to OS |
| Scratch Begin | `ScratchBegin()` | `mem.scratch.begin()` | Acquire thread-local checkpoint |
| Scratch End | `ScratchEnd(temp)` | `mem.scratch.end(temp)` | Roll back thread-local checkpoint |
| Create Pool | `PoolCreate(item_sz, count)` | `pool.create(item_sz, count)` | Pre-allocate fixed-size slab pool |
| Pool Alloc | `PoolAlloc(pool)` | `pool.alloc(pool)` | Pop slot from free-list |
| Pool Free | `PoolFree(pool, ptr)` | `pool.free(pool, ptr)` | Return slot to free-list |
| Destroy Pool | `PoolDestroy(pool)` | `pool.destroy(pool)` | Free slab pool back to OS |

---

## 5. Use Cases
1. **Per-Frame Game / Render Loops**: Allocate all temporary transforms, transient entity buffers, and rendering commands from an `Arena`, resetting it at the end of each frame (`ArenaReset`) with 0 kernel syscalls.
2. **Compiler & AST Parsing**: Allocate abstract syntax tree nodes into an arena during a compilation pass. When compilation completes or errors out, destroy the entire tree in 1 cycle.
3. **High-Frequency Network Packets**: Pre-allocate a `Pool` of 4096 network packet descriptors (1500 bytes each) for deterministic $O(1)$ packet reception.

---

## 6. Complete C11 Usage Examples

### Example 1: Frame-Based Scratch Buffer
```c
#include "cplus/cplus.h"

void process_sensor_batch(const char *raw_data) {
    TempArena scratch = ScratchBegin();
    
    usize count = 0;
    StrView *readings = StrSplitArena(StrFromC(raw_data), ',', scratch.arena, &count);
    
    float sum = 0.0f;
    for (usize i = 0; i < count; i++) {
        sum += (float)atof(readings[i].data);
    }
    printf("Processed %zu sensors: Average = %.2f\n", count, sum / (float)count);
    
    ScratchEnd(scratch); // All memory reclaimed instantly
}
```

### Example 2: SIMD-Aligned Matrix Processing
```c
#include "cplus/cplus.h"

void compute_avx_matrix(Arena *arena) {
    // 32-byte alignment required for AVX-256 load/store instructions
    float *matrix_a = (float *)ArenaAllocAlign(arena, 1024 * sizeof(float), 32);
    float *matrix_b = (float *)ArenaAllocAlign(arena, 1024 * sizeof(float), 32);
    
    // Process matrices with zero cacheline unaligned penalty
    for (int i = 0; i < 1024; i++) {
        matrix_a[i] = (float)i;
        matrix_b[i] = matrix_a[i] * 2.0f;
    }
}
```

---

## 7. Limitations & Operational Constraints
- **No Individual `free()` in Arena**: An individual object cannot be freed independently of the arena. If you require arbitrary individual element deallocation, use `Pool` instead.
- **Thread Confinement**: An `Arena` is not thread-safe. Multiple worker threads must not allocate from the same `Arena` without an external `Mutex` or `ScopedLock`. Use `ScratchBegin()` to obtain isolated per-thread arenas.
- **Maximum Chunk Size**: Default chunk size is clamped between 4 KiB and 1 GiB.
