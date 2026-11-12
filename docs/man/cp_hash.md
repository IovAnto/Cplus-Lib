# Module Specification: cp_hash (Hashing Subsystem)

## 1. TL;DR
The `cp_hash` subsystem delivers both ultra-high-throughput non-cryptographic hash functions for hash tables and checksums, and a clean FIPS 180-4 compliant SHA-256 implementation with zero external dependencies.

```c
#include "cplus/cplus.h"

/* 1. Ultra-Fast 64-bit Mixer Hash (< 1 ns/byte, > 3.5 GB/s) */
const char *key = "session_token_991823";
u64 h64 = Hash64(key, strlen(key));

/* 2. Standard 32-bit FNV-1a */
u32 h32 = HashFnv1a32(key, strlen(key));

/* 3. Cryptographic FIPS 180-4 SHA-256 */
u8 digest[32];
char hex[65];
Sha256(key, strlen(key), digest);
Sha256Hex(digest, hex);
printf("SHA-256: %s\n", hex);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Fast Non-Cryptographic Mixer: `Hash64` (`cp_hash64`)
- Employs an unrolled multiply-shift-xor mixing pipeline based on 64-bit golden ratio constants.
- Processes 8-byte words per step with residual handling for trailing bytes.
- Passes standard Avalanche tests with uniform distribution across 64-bit hash table buckets.

### 2.2 FNV-1a: `HashFnv1a32` & `HashFnv1a64`
- Standard Fowler-Noll-Vo algorithm.
- Initialized with offset basis (`2166136261u` for 32-bit, `14695981039346656037ull` for 64-bit).
- Multiplies by FNV prime per byte and XORs with byte payload.
- Minimal code footprint, optimal for short strings and identifiers.

### 2.3 Cryptographic SHA-256: `Sha256` (`cp_sha256`)
```c
typedef struct {
    u32 state[8];
    u64 count;
    u8 buffer[64];
} cp_sha256_ctx_t;
```
- Fully compliant with NIST FIPS PUB 180-4.
- Processes 512-bit message blocks with 64 constant transformation rounds.
- Supports both incremental streaming (`Sha256Init` -> `Sha256Update` -> `Sha256Final`) and direct one-shot evaluation (`Sha256`).
- Converts 32-byte binary digest to lowercase hexadecimal string (`Sha256Hex`).

---

## 3. Operational Guarantees & Contracts

| Function | Output Size | Throughput | Cryptographic Security | Heap Allocations |
|---|---|---|---|---|
| `Hash64` | 64 bits | ~3.8 GB/s (memory bound) | No | 0 |
| `HashFnv1a64` | 64 bits | ~1.2 GB/s | No | 0 |
| `HashFnv1a32` | 32 bits | ~1.2 GB/s | No | 0 |
| `Sha256` | 256 bits | ~350 MB/s | Yes (Pre-image resistant) | 0 |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Fast 64-bit Hash | `Hash64(data, len)` | `hash_api.u64_hash(data, len)` | High-throughput non-cryptographic 64-bit mixer hash |
| FNV-1a 32-bit | `HashFnv1a32(data, len)` | `hash_api.fnv1a32(data, len)` | Standard 32-bit Fowler-Noll-Vo hash |
| FNV-1a 64-bit | `HashFnv1a64(data, len)` | `hash_api.fnv1a64(data, len)` | Standard 64-bit Fowler-Noll-Vo hash |
| One-Shot SHA-256 | `Sha256(data, len, out)` | `hash_api.sha256(data, len, out)` | Compute NIST FIPS 180-4 32-byte hash digest |
| SHA-256 to Hex | `Sha256Hex(in, out_hex)` | `hash_api.sha256Hex(in, out_hex)` | Format 32-byte hash into 64-character null-terminated hex |
| Init SHA-256 Ctx | `Sha256Init(ctx)` | `cp_sha256_init(ctx)` | Initialize incremental SHA-256 context |
| Update SHA-256 | `Sha256Update(ctx, buf, len)`| `cp_sha256_update(ctx, buf, len)`| Stream bytes into SHA-256 context |
| Finalize SHA-256 | `Sha256Final(ctx, out)` | `cp_sha256_final(ctx, out)` | Complete padding and emit 32-byte digest |

---

## 5. Use Cases
1. **Hash Table Keying**: Use `Hash64` inside custom hash tables or `HashMap` for minimal collision rates and high throughput.
2. **File Integrity Verification**: Hash incoming firmware or file buffers with `Sha256` and compare checksums against authoritative sources.
3. **Data Deduplication**: Identify identical data chunks in network transmission by maintaining a set of SHA-256 hashes.
4. **Fast String Partitioning**: Partition short identifiers across thread worker queues using `HashFnv1a32(id, len) % worker_count`.

---

## 6. Complete Compilable Example

```c
#include "cplus/cplus.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    const char *msg = "The quick brown fox jumps over the lazy dog";
    usize len = strlen(msg);

    /* 1. Fast mixer hash */
    u64 h64 = hash_api.u64_hash(msg, len);
    assert(h64 != 0);

    /* 2. One-shot SHA-256 */
    u8 digest[32];
    hash_api.sha256(msg, len, digest);

    char hex[65];
    hash_api.sha256Hex(digest, hex);
    assert(strcmp(hex, "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592") == 0);
    printf("Message: \"%s\"\nSHA-256: %s\n", msg, hex);

    /* 3. Incremental streaming SHA-256 */
    cp_sha256_ctx_t ctx;
    Sha256Init(&ctx);
    Sha256Update(&ctx, "The quick brown ", 16);
    Sha256Update(&ctx, "fox jumps over the lazy dog", 27);
    
    u8 stream_digest[32];
    Sha256Final(&ctx, stream_digest);

    char stream_hex[65];
    Sha256Hex(stream_digest, stream_hex);
    assert(strcmp(hex, stream_hex) == 0);

    return 0;
}
```

---

## 7. Limitations & Edge Cases
- **Non-Cryptographic Collisions**: `Hash64` and `HashFnv1a` are strictly designed for hash tables, bloom filters, and checksums. They must not be used for cryptographic signatures, passwords, or security authentication.
- **Hex Buffer Sizing**: `Sha256Hex` requires an output buffer of at least 65 bytes (`32 * 2 + 1` for null-terminator). Supplying smaller buffers causes memory corruption.
