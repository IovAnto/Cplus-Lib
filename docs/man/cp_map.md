# Module Specification: cp_map (Open-Addressing Hash Table)

## 1. TL;DR
The `cp_map` module implements a high-performance open-addressing hash table using linear probing, power-of-two bitmask indexing, and tombstone recycling.

```c
#include "cplus/cplus.h"

Arena *arena = ArenaCreate(64 * 1024);
HashMap *map = MapCreate(32, arena);

MapSet(map, "port", (void *)8080);
MapSet(map, "host", (void *)"localhost");

int port = (int)(intptr_t)MapGet(map, "port");
assert(MapContains(map, "host"));

ArenaDestroy(arena); // Bulk frees the map and all entries
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Struct Layout: `HashMap` (`cp_map_t`) and `MapEntry` (`cp_map_entry_t`)
```c
typedef struct {
    const char *key;
    void *value;
    u64 hash;
    bool deleted;
} MapEntry;

typedef struct cp_map {
    MapEntry *entries;
    usize capacity;
    usize count;
    cp_arena_t *arena;
} HashMap;
```
- **L1 Cache Friendly Linear Probing**: All slots reside in a single contiguous array of `MapEntry` structures. Probing consecutive cachelines minimizes DRAM stalls compared to separate chaining with linked lists.
- **Power-of-Two Masking**: The initial capacity is clamped to the next power of two. Slot indexing uses bitwise AND (`hash & (capacity - 1)`) instead of integer division (`hash % capacity`).
- **Tombstone Recycling**: When an element is removed via `MapRemove`, the slot is marked `deleted = true`. Subsequent insertions reuse tombstone slots, preventing artificial table bloat.
- **Dynamic Re-hashing**: When load factor exceeds 70% (`count * 100 / capacity >= 70`), the table doubles its capacity and re-inserts all active entries.

---

## 3. Operational Guarantees & Contracts

| Metric | Guarantee | Notes |
|---|---|---|
| **Lookup Latency** | $O(1)$ average, $O(N)$ worst-case | Bounded by linear probe chain |
| **Insertion Latency** | $O(1)$ amortized | Triggers rehash at >70% load factor |
| **Deletion Latency** | $O(1)$ | Marks tombstone slot |
| **Memory Layout** | Contiguous table | Single allocation on Arena or Heap |
| **Thread-Safety** | Safe for concurrent reads | Requires `ScopedLock` for concurrent writes |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Create Map | `MapCreate(init_cap, arena)` | `map.create(init_cap, arena)` | Instantiate new hash map |
| Set Key-Value | `MapSet(map, key, val)` | `map.set(map, key, val)` | Insert or update key |
| Get Value | `MapGet(map, key)` | `map.get(map, key)` | Retrieve void* value (NULL if missing) |
| Remove Key | `MapRemove(map, key)` | `map.remove(map, key)` | Delete key and set tombstone |
| Contains Key | `MapContains(map, key)` | `map.contains(map, key)` | Check existence (returns bool) |
| Clear Map | `MapClear(map)` | `map.clear(map)` | Reset count to 0 |
| Entry Count | `MapCount(map)` | `map.count(map)` | Number of active key-value pairs |
| Table Capacity| `MapCap(map)` | `map.cap(map)` | Current allocated slot capacity |
| Destroy Map | `MapDestroy(map)` | `map.destroy(map)` | Deallocate table memory |

---

## 5. Use Cases
1. **HTTP Routing Tables**: Route incoming URL path strings to handler function pointers.
2. **Symbol Tables & AST Environments**: Map variable identifiers to type/value descriptors during compilation.
3. **Session & Cache Registries**: Track active network connections by socket descriptor string.

---

## 6. Complete C11 Usage Examples

### Example: Configuration Registry
```c
#include "cplus/cplus.h"

void load_and_query_config(void) {
    TempArena scratch = ScratchBegin();
    HashMap *cfg = MapCreate(16, scratch.arena);
    
    MapSet(cfg, "db_host", (void *)"127.0.0.1");
    MapSet(cfg, "db_port", (void *)(intptr_t)5432);
    MapSet(cfg, "max_connections", (void *)(intptr_t)100);
    
    if (MapContains(cfg, "db_host")) {
        const char *host = (const char *)MapGet(cfg, "db_host");
        int port = (int)(intptr_t)MapGet(cfg, "db_port");
        printf("Connecting to PostgreSQL at %s:%d\n", host, port);
    }
    
    ScratchEnd(scratch); // Automatically frees the entire hash map
}
```

---

## 7. Limitations & Operational Constraints
- **Key Lifetime**: `MapSet` does not duplicate the `const char *key` string when using heap allocation. Ensure the key string remains valid for the lifetime of the map, or use `Arena` to allocate both keys and the map together.
- **NULL Value Sentinel**: `MapGet` returns `NULL` when a key does not exist. Do not store raw `NULL` as a valid value if you need to differentiate missing keys from null values; use `MapContains` instead.
