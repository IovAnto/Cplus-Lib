# Module Specification: cp_algo (Algorithms & Fast PRNG)

## 1. TL;DR
The `cp_algo` module provides high-performance IntroSort ($O(N \log N)$ worst-case), linear $O(N)$ RadixSort for 32-bit unsigned integers, binary search, array permutation utilities, and the lock-free Xoshiro256** pseudo-random generator.

```c
#include "cplus/cplus.h"

/* 1. Fast Linear O(N) Radix Sort */
u32 keys[] = { 9821, 102, 54, 88123, 4, 712 };
AlgoRadixSortU32(keys, 6);

/* 2. Binary Search in Sorted Array */
isize idx = AlgoBinarySearchU32(keys, 6, 102);
assert(idx != -1);

/* 3. Sub-Nanosecond PRNG (Xoshiro256**) */
Rng rng;
RandInit(&rng, 123456789ULL);
u64 random_val = RandU64(&rng);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 IntroSort Hybrid Architecture (`AlgoSort`)
- Combines three sorting techniques:
  1. **QuickSort**: Used for initial partitioning with median-of-three pivot selection.
  2. **HeapSort Fallback**: If recursion depth exceeds $2 \times \lfloor\log_2 N\rfloor$, IntroSort switches to HeapSort to prevent worst-case $O(N^2)$ quadratic degradation.
  3. **InsertionSort**: Partitions smaller than 16 elements switch to inlined InsertionSort for optimal instruction pipeline utilization.

### 2.2 Linear RadixSort (`AlgoRadixSortU32`)
- Least-Significant-Digit (LSD) Radix Sort processing 8 bits per pass (4 passes total for 32-bit words).
- Uses 256 counting histogram buckets per pass, sorting arrays in strictly linear $O(N)$ time.

### 2.3 Xoshiro256** PRNG (`Rng` / `cp_rand_t`)
```c
typedef struct {
    u64 s[4];
} Rng;
```
- Passes all standard BigCrush statistical tests. Generates 64-bit random values in ~0.8 nanoseconds with 0 locks.

---

## 3. Operational Guarantees & Contracts

| Metric | IntroSort | RadixSort U32 | Binary Search | Xoshiro256** |
|---|---|---|---|---|
| **Time Complexity** | $O(N \log N)$ guaranteed | $O(N)$ strictly linear | $O(\log N)$ | $O(1)$ (< 1 ns/op) |
| **Auxiliary Memory** | $O(\log N)$ stack frames | 1 scratch buffer | $O(1)$ | 32 bytes on stack |
| **Stability** | Unstable | Stable | N/A | Deterministic |
| **Thread-Safety** | Reentrant | Reentrant | Pure function | Isolated per state |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Generic Sort | `AlgoSort(base, n, sz, cmp)`| `algo.sort(...)` | IntroSort hybrid sorter |
| Sort i32 | `AlgoSortI32(arr, len)` | `algo.sortI32(arr, len)` | Inlined integer sort |
| Sort u32 | `AlgoSortU32(arr, len)` | `algo.sortU32(arr, len)` | Inlined unsigned integer sort |
| Sort f32 | `AlgoSortF32(arr, len)` | `algo.sortF32(arr, len)` | Inlined float sort |
| Radix Sort | `AlgoRadixSortU32(arr, len)`| `algo.radixSortU32(...)`| O(N) linear LSD radix sort |
| Binary Search i32 | `AlgoBsearchI32(arr, len, k)`| `algo.bsearchI32(...)` | Binary search (returns -1 if not found) |
| Binary Search u32 | `AlgoBsearchU32(arr, len, k)`| `algo.bsearchU32(...)` | Binary search on unsigned array |
| Shuffle Array | `AlgoShuffle(base, n, sz)` | `algo.shuffle(...)` | In-place Fisher-Yates shuffle |
| Reverse Array | `AlgoReverse(base, n, sz)` | `algo.reverse(...)` | In-place element reversal |
| Init PRNG | `RandInit(rng, seed)` | `rng.init(rng, seed)` | Seed Xoshiro256** generator |
| Random u64 | `RandU64(rng)` | `rng.u64_(rng)` | Generate 64-bit unsigned random value |
| Random f64 | `RandF64(rng)` | `rng.f64_(rng)` | Generate [0.0, 1.0) double float |
| Random Range | `RandRange(rng, min, max)` | `rng.range(rng, min, max)`| Uniform integer in [min, max] |

---

## 5. Use Cases
1. **Telemetry & Sensor Sorting**: Sort 1,000,000 integer readings in sub-millisecond time using `AlgoRadixSortU32`.
2. **Monte Carlo Simulations**: Generate billions of pseudorandom numbers per second using thread-local `Rng` states.
3. **Database Index Lookup**: Search binary sorted keys with $O(\log N)$ guarantees.

---

## 6. Complete C11 Usage Examples

### Example: Sorting with Anonymous Lambda
```c
#include "cplus/cplus.h"

typedef struct {
    int id;
    float score;
} Player;

void rank_players(Player *players, usize count) {
    AlgoSort(players, count, sizeof(Player), Lambda(int, (const void *a, const void *b), {
        float diff = ((const Player *)b)->score - ((const Player *)a)->score;
        return (diff > 0.0f) - (diff < 0.0f);
    }));
}
```

---

## 7. Limitations & Operational Constraints
- **Array Monotonicity in `BinarySearch`**: `AlgoBsearchI32` requires the input array to be sorted in ascending order beforehand.
- **Zero Seed in PRNG**: Do not seed `Rng` with all zeros (`seed = 0`), as Xoshiro256** requires at least one non-zero state bit. `RandInit` automatically mixes non-zero bits via SplitMix64 if seed is 0.
