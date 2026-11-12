# Module Specification: cp_core (Core Foundations, RAII & Intrinsics)

## 1. TL;DR
The `cp_core` module forms the foundational substrate for all `C+` subsystems. It establishes exact-width scalar types, RAII cleanup macros (`Defer`, `DeferAction`), zero-overhead anonymous lambdas (`Lambda`), C11 `_Generic` polymorphic printing (`Print`, `Println`), hardware bit manipulation intrinsics, and ANSI diagnostic logging.

```c
#include "cplus/cplus.h"

/* 1. Exact-Width Types & C11 Polymorphic Print */
u64 count = 42;
f64 ratio = 3.14159;
Println(count); // Automatically dispatches unsigned long long formatter
Println(ratio); // Automatically dispatches double formatter

/* 2. RAII Scope Deferral */
FILE *fp = fopen("data.bin", "rb");
if (fp) {
    Defer(fclose, fp); // Automatically invoked upon block exit
    // ... work with fp ...
}

/* 3. Hardware Bit Manipulation */
u32 set_bits = BitPopcount32(0x00FF00FF); // 16
u64 next_p2 = BitNextPow2(300);          // 512

/* 4. Diagnostic Logging */
LogInfo("Subsystem initialized with %u cores", OsNumCpus());
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Ergonomic Fixed-Width Types
| Type | Underlying C Type | Size | Purpose |
|---|---|---|---|
| `u8`, `u16`, `u32`, `u64` | `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` | 1, 2, 4, 8 bytes | Unsigned integers |
| `i8`, `i16`, `i32`, `i64` | `int8_t`, `int16_t`, `int32_t`, `int64_t` | 1, 2, 4, 8 bytes | Signed integers |
| `usize` | `size_t` | 4 or 8 bytes | Memory capacity and array indexing |
| `isize` | `ptrdiff_t` | 4 or 8 bytes | Signed pointer distance and offsets |
| `f32`, `f64` | `float`, `double` | 4, 8 bytes | IEEE 754 floating point |
| `b8`, `b32` | `uint8_t`, `uint32_t` | 1, 4 bytes | Explicit-width boolean storage |

### 2.2 RAII Cleanup: `Defer` & `DeferAction`
```c
typedef struct {
    void (*fn)(void *);
    void *arg;
} cp_defer_t;

#define Defer(func_, arg_) \
    __attribute__((cleanup(_cp_defer_cleanup))) cp_defer_t CP_CONCAT(_cp_defer_, __COUNTER__) = { \
        .fn = (void (*)(void *))(func_), \
        .arg = (void *)(arg_) \
    }
```
- Employs GCC and Clang `__attribute__((cleanup))` to register a destructor function on the stack.
- Executes predictably in LIFO order upon exiting the enclosing lexical scope (including early returns, breaks, or error paths).
- Allocates zero bytes on the heap.

### 2.3 Anonymous Inline Lambdas: `Lambda`
```c
#if defined(__GNUC__) && !defined(__clang__)
#define CP_HAS_LAMBDA 1
#define Lambda(ret_type, args, body) __extension__ ({ ret_type _cp_lambda args body _cp_lambda; })
#else
#define CP_HAS_LAMBDA 0
#endif
```
- Employs GNU C statement expressions containing a nested function declaration.
- Compiles down to an ordinary function pointer with 0 runtime trampolines and 0 heap allocation.
- In Clang or non-GCC compilers (`CP_HAS_LAMBDA == 0`), standard static functions or iteration macros (`VecEach`, `VecMap`) are used.

### 2.4 C11 Polymorphic Printing: `Print` & `Println`
- Implemented using C11 `_Generic` compile-time type dispatch.
- Emits formatted strings without runtime type introspection or `printf` format string vulnerabilities.
- Handles `bool`, `char`, signed integers (`i8`-`i64`), unsigned integers (`u8`-`u64`), floating point (`f32`, `f64`), strings (`char *`, `const char *`), and pointers (`void *`).

### 2.5 Hardware Bit Intrinsics
- Maps directly to target CPU single-cycle instructions via compiler builtins:
  - `BitPopcount`: `POPCNT` instruction on x86-64.
  - `BitClz`: `LZCNT` / `BSR` (Count Leading Zeros).
  - `BitCtz`: `TZCNT` / `BSF` (Count Trailing Zeros).
  - `BitBswap`: `BSWAP` (Byte Swap Endianness).
  - `BitRotl` / `BitRotr`: Hardware bitwise rotation instructions.
  - `BitIsPow2` / `BitNextPow2`: Zero-branch bitwise power-of-two arithmetic.

---

## 3. Operational Guarantees & Contracts

| Operation | Implementation | Runtime Overhead | Heap Allocations |
|---|---|---|---|
| `Defer` / `DeferAction` | Stack `__attribute__((cleanup))` | 0 cycles until scope exit | 0 |
| `Lambda` | GNU Statement Expression | 0 cycles (direct function pointer) | 0 |
| `Print` / `Println` | C11 `_Generic` compile-time dispatch | Direct `fputs` / `printf` call | 0 |
| `BitPopcount` / `Clz` / `Ctz` | Compiler builtin intrinsic | 1 CPU clock cycle | 0 |
| `BitBswap16/32/64` | Compiler builtin intrinsic | 1 CPU clock cycle | 0 |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Defer (1 Arg) | `Defer(func, arg)` | - | Execute cleanup function with argument on scope exit |
| Defer Action | `DeferAction(func)` | - | Execute parameterless cleanup function on scope exit |
| Anonymous Lambda | `Lambda(ret, args, body)`| - | Construct anonymous inline function pointer (GCC) |
| Print Value | `Print(val)` | - | C11 type-safe polymorphic print to stdout |
| Println Value | `Println(val)` | - | C11 type-safe polymorphic print followed by newline |
| Assert Message | `AssertMsg(cond, msg)` | - | Verify assertion; abort with file/line on failure |
| Panic | `Panic(msg)` | - | Print fatal error diagnostic and abort process |
| Popcount 32-bit | `BitPopcount32(x)` | `bit.popcount32(x)` | Count set bits in 32-bit word |
| Popcount 64-bit | `BitPopcount64(x)` | `bit.popcount64(x)` | Count set bits in 64-bit word |
| Count Leading Zeros | `BitClz32(x)` / `64(x)` | `bit.clz32(x)` / `clz64` | Count leading zero bits |
| Count Trailing Zeros | `BitCtz32(x)` / `64(x)` | `bit.ctz32(x)` / `ctz64` | Count trailing zero bits |
| Byte Swap | `BitBswap16/32/64(x)` | `bit.bswap16/32/64` | Reverse byte endianness |
| Rotate Left | `BitRotl32(x, s)` / `64`| `bit.rotl32(x, s)` / `64` | Rotate bits to the left |
| Rotate Right | `BitRotr32(x, s)` / `64`| `bit.rotr32(x, s)` / `64` | Rotate bits to the right |
| Is Power of 2 | `BitIsPow2(x)` | `bit.isPow2(x)` | Verify if unsigned integer is power of 2 |
| Next Power of 2 | `BitNextPow2(x)` | `bit.nextPow2(x)` | Calculate smallest power of 2 >= x |
| Set Log Level | `LogSetLevel(lvl)` | `cp_log_set_level(lvl)` | Filter logging threshold |
| Log Debug / Info | `LogDebug(...)` / `LogInfo`| - | Formatted ANSI colored diagnostic output |
| Log Warn / Error | `LogWarn(...)` / `LogError`| - | Formatted ANSI colored warning/error output |

---

## 5. Use Cases
1. **Resource Lifetime Safety**: Ensure sockets, file descriptors, arenas, and mutexes are released unconditionally via `Defer`.
2. **Network Protocol Serialization**: Reorder multi-byte packet header fields into big-endian network order using `BitBswap16` and `BitBswap32`.
3. **Capacity Exponential Growth**: Round arbitrary allocation requests up to the nearest power of two using `BitNextPow2` for cache and ring buffer indexing.
4. **Fast Bitset Iteration**: Enumerate active flags in bitmasks using `BitCtz64` and bit-clearing `x &= x - 1`.

---

## 6. Complete Compilable Example

```c
#include "cplus/cplus.h"
#include <stdio.h>
#include <assert.h>

static void cleanup_resource(void *arg) {
    int *res = (int *)arg;
    *res = 0;
}

int main(void) {
    /* 1. RAII Deferral */
    int active = 1;
    {
        Defer(cleanup_resource, &active);
        assert(active == 1);
    }
    assert(active == 0); // Cleanup executed automatically on block exit

    /* 2. Bit manipulation */
    u32 mask = 0x000F0000;
    assert(bit.popcount32(mask) == 4);
    assert(bit.clz32(mask) == 12);
    assert(bit.ctz32(mask) == 16);
    assert(bit.isPow2(1024) == true);
    assert(bit.nextPow2(1025) == 2048);
    assert(bit.bswap32(0x12345678) == 0x78563412);

    /* 3. C11 Generic Polymorphic Println */
    Println("--- C+ Core Verification ---");
    Println(true);
    Println(1234567890LL);
    Println(3.14159);

    /* 4. Logging */
    LogSetLevel(CP_LOG_LEVEL_INFO);
    LogInfo("Core subsystem operational, next pow2 of 45 is %llu", (unsigned long long)bit.nextPow2(45));

    return 0;
}
```

---

## 7. Limitations & Edge Cases
- **Scope Order**: `Defer` statements in the same lexical block execute in strict LIFO (reverse) order. Ensure dependent resources are deferred after their prerequisites.
- **`BitClz` and `BitCtz` with 0**: Calling `BitClz32(0)` returns 32 and `BitCtz32(0)` returns 32 (or 64 for 64-bit variants). Unlike raw compiler builtins which produce undefined behavior for zero on some architectures, `C+` explicitly guards zero inputs to return the word bit length safely.
