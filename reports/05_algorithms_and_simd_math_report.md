# Scientific Experiment Report: High-Speed Algorithms & SIMD Linear Algebra
**Experiment ID:** EXP-05-ALGO-MATH  
**Module Tested:** `cp_algo` (IntroSort, RadixSort, Binary Search) & `cp_math` (SIMD Mat4, Vec2/3/4)  
**Date:** 2026-11-12  
**Status:** VALIDATED, BIT-EXACT PARITY CONFIRMED & REPRODUCIBLE  

---

## 1. Experimental Environment & Hardware Specifications

| Parameter | Specification |
| :--- | :--- |
| **Processor (CPU)** | Intel(R) Core(TM) i5-7360U CPU @ 2.30GHz |
| **Topology** | 2 physical cores, 4 threads |
| **Vector Registers** | 256-bit AVX2, 128-bit SSE4.2 (16 XMM/YMM registers) |
| **L1 Data Cache** | 64 KiB (2 x 32 KiB) |
| **L1 Instruction Cache** | 64 KiB (2 x 32 KiB) |
| **L2 Cache** | 512 KiB (2 x 256 KiB) |
| **L3 Shared Cache** | 4.0 MiB |
| **System Memory (RAM)** | 7.6 GiB LPDDR3 |
| **Operating System** | Linux 7.2.14-arch1-1 x86_64 |
| **Compiler** | GCC 16.2.1 20260810 / Clang 23.1.4 |
| **Optimization Flags** | `-O3 -march=native -ffunction-sections -fdata-sections -Wl,--gc-sections` |
| **Timing Clock** | `clock_gettime(CLOCK_MONOTONIC)` (nanosecond resolution) |

---

## 2. Strict Parity & Correctness Verification

Prior to benchmark execution, both algorithm modules underwent mathematical parity verification:

### 2.1 Sorting Algorithms Parity (1,000,000 integers)
* **Sorted Range (`qsort`):** `[2,490 ... 2,147,476,386]`
* **Sorted Range (`IntroSort`):** `[2,490 ... 2,147,476,386]`
* **Sorted Range (`RadixSort`):** `[2,490 ... 2,147,476,386]`
* **Parity Status:** **100.0% IDENTICAL across all 1,000,000 elements.**

### 2.2 4x4 Matrix Multiplication Parity (SIMD vs Scalar)
* **Maximum cell absolute difference:** **`0.00e+00`**
* **Parity Status:** **100.0% BIT-EXACT MATCH.**

---

## 3. Experimental Results

### 3.1 Part A: Sorting 1,000,000 Integers ($N = 5$ Runs)

| Run # | Pure C `qsort` ($ms$) | Cplus `IntroSort` ($ms$) | Cplus `RadixSort` ($ms$) |
| :---: | :---: | :---: | :---: |
| **Run 1** | 193.23 | 111.18 | 9.78 |
| **Run 2** | 190.95 | 111.33 | 12.01 |
| **Run 3** | 189.94 | 111.27 | 10.48 |
| **Run 4** | 192.42 | 113.88 | 10.48 |
| **Run 5** | 190.86 | 111.10 | 10.43 |

#### Statistical Summary (Part A):
| Implementation | Mean Time ($\mu$) | Min / Max | StdDev ($\sigma$) | Latency / Elem | Throughput | Speedup vs `qsort` |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Pure C (`qsort` with callback)** | **191.48 ms** | 189.94 / 193.23 ms | $\pm 1.18\text{ ms}$ | **191.48 ns** | $5.22\text{ M elem/s}$ | **1.00x** (Baseline) |
| **Cplus `IntroSort` (Inlined)** | **111.75 ms** | 111.10 / 113.88 ms | $\pm \mathbf{1.07\text{ ms}}$ | **111.75 ns** | **$8.95\text{ M elem/s}$** | **1.71x FASTER** |
| **Cplus `RadixSort` (Linear $O(N)$)** | **10.64 ms** | **9.78 / 12.01 ms** | $\pm \mathbf{0.74\text{ ms}}$ | **10.64 ns** | **$93.98\text{ M elem/s}$** | **18.00x FASTER** |

---

### 3.2 Part B: 4x4 Matrix Multiplication (5,000,000 operations, $N = 5$)

| Run # | Pure C Scalar 3-Loop ($ms$) | Cplus SIMD SSE/AVX Vectorized ($ms$) |
| :---: | :---: | :---: |
| **Run 1** | 45.31 | 18.87 |
| **Run 2** | 45.72 | 19.09 |
| **Run 3** | 47.08 | 18.89 |
| **Run 4** | 45.68 | 19.52 |
| **Run 5** | 45.62 | 31.12 |

#### Statistical Summary (Part B):
| Implementation | Mean Time ($\mu$) | Min / Max | StdDev ($\sigma$) | Latency / Mul | Throughput | Speedup vs Pure C |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Pure C Scalar Loop** | **45.88 ms** | 45.31 / 47.08 ms | $\pm 0.62\text{ ms}$ | **9.18 ns** | $108.98\text{ M mul/s}$ | **1.00x** (Baseline) |
| **Cplus SIMD Vectorized** | **21.50 ms** | **18.87 / 31.12 ms** | $\pm 4.82\text{ ms}$ | **4.30 ns** | **$232.55\text{ M mul/s}$** | **2.13x FASTER** |

---

## 4. Key Scientific Insights

1. **Elimination of Function-Pointer Callbacks in Sorting:**
   * Standard library `qsort` suffers an indirect jump (`call *%rax`) on every single element comparison, completely defeating branch prediction.
   * `IntroSort` inlines comparisons directly, delivering a **1.71x speedup** on general data.
   * `RadixSort` leverages an $O(N)$ cache-friendly 8-bit LSD pass, delivering an astonishing **18.00x speedup** ($10.64\text{ ms}$ vs $191.48\text{ ms}$).
2. **Sub-5-Nanosecond Vectorized Matrix Multiplies:**
   * By keeping matrix columns packed into 128-bit XMM registers (`_mm_loadu_ps`, `_mm_shuffle_ps`, `_mm_mul_ps`, `_mm_add_ps`), our vectorized 4x4 multiplication operates in **4.30 nanoseconds** per full 4x4 matrix, achieving **$232\text{ million operations/second}$** with a **2.13x speedup** over scalar C.

---

## 5. Verification & Memory Safety Artifacts

* **Unit Test Status:** All test assertions passed (`test_algorithms`, `test_simd_linear_algebra`).
* **Sanitizer Verification:** AddressSanitizer and UndefinedBehaviorSanitizer verified 0 memory leaks, 0 undefined behaviors.
* **Raw Logs:**
  * Benchmark Raw Log: `reports/raw_logs/step05_algo_math_raw.log`
  * Unit Test Log: `reports/raw_logs/step05_unit_tests.log`
  * ASan Log: `reports/raw_logs/step05_asan_tests.log`
