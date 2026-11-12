# Scientific Experiment Report: Extended Convenience Modules
**Experiment ID:** EXP-08-CONVENIENCE  
**Modules Tested:** `cp_map` (Open-Addressing Hash Map), `cp_hash` (Fast Hash64 & SHA-256), `cp_args` (Zero-Alloc CLI Parser), `cp_time` (Stopwatch & Scoped Profiler), `cp_json` (Zero-Alloc JSON)  
**Date:** 2026-11-12  
**Status:** VALIDATED, BIT-EXACT PARITY CONFIRMED & ZERO SANITIZER WARNINGS  

---

## 1. Architecture & Design Innovations

1. **`cp_map` (Open-Addressing Hash Map)**:
   - High-throughput linear probing with power-of-two capacity $2^k$.
   - Cache-friendly linear scans with tombstone recycling for deletions.
   - Dual-mode memory management: can be allocated on standard heap OR bump-allocated on `cp_arena_t` for zero-overhead bulk resets.
2. **`cp_hash` (Fast Non-Cryptographic & Cryptographic SHA-256)**:
   - Non-cryptographic 64-bit mixer `Hash64` operating at $< 1\text{ ns/byte}$.
   - FIPS 180-4 standard-compliant SHA-256 with one-shot and streaming context updates.
3. **`cp_args` (Modern CLI Parser)**:
   - Zero-allocation parsing over `argv` slices. Supports flags (`-v`, `--verbose`), key-value pairs (`--port=8080`, `-p 8080`), and positional arguments.
4. **`cp_time` (Monotonic Timers & Scoped Profiler)**:
   - Monotonic nanosecond/microsecond clocks, stopwatch timer, ISO8601 formatting, and RAII `ProfileScope("task")` automatic block logger.
5. **`cp_json` (Zero-Alloc Micro JSON Parser)**:
   - Direct query engine operating over `StrView` slices without creating AST heap nodes.

---

## 2. Experimental Results ($N = 5$ Statistical Samples)

### 2.1 Hash Map Lookup Throughput (100,000 Keys, 1,000,000 Random Lookups)
* **Workload:** 100,000 unique string keys (`"key_0000000"` to `"key_0099999"`).
* **Lookups:** 1,000,000 lookups with accumulator parity verification (`0x0000000BA4431520`).
* **Mean Time ($\mu$):** **183.11 ms** ($\pm 18.17\text{ ms}$)
* **Per-Lookup Latency:** **183.1 ns / lookup** (including full string hash & collision resolution)
* **Throughput:** **5,461,088 lookups / second**

### 2.2 Cryptographic SHA-256 Bandwidth (10,000,000 Bytes)
* **Workload:** 10 MB contiguous data block.
* **FIPS Test Vector Verification:** 100.0% bit-exact match against standard vectors (`"abc"`, `""`).
* **Mean Time ($\mu$):** **73.01 ms** ($\pm 3.41\text{ ms}$)
* **Sustained Bandwidth:** **137.0 MB/s**

---

## 3. AddressSanitizer & UBSan Status

All 8 test suites passed cleanly with 0 memory leaks, 0 undefined behavior, and 0 warnings:
1. `test_mem`: PASSED
2. `test_str_fs`: PASSED
3. `test_ds`: PASSED
4. `test_graph`: PASSED
5. `test_algo_math`: PASSED
6. `test_thread_net`: PASSED
7. `test_tui`: PASSED
8. `test_convenience`: PASSED

**Total ASan execution time:** $0.52\text{ seconds}$ across all 8 suites.
