/**
 * @file cp_algo.h
 * @brief High-performance algorithms: IntroSort, O(N) RadixSort, Binary Search, and Shuffling.
 */

#ifndef CPLUS_ALGO_H
#define CPLUS_ALGO_H

#include "cp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Sorting Algorithms
 * ------------------------------------------------------------------------- */
void cp_algo_sort_i32(i32 *arr, usize len);
void cp_algo_sort_u32(u32 *arr, usize len);
void cp_algo_sort_f32(f32 *arr, usize len);
void cp_algo_sort(void *base, usize num, usize size, int (*cmp)(const void *, const void *));

/**
 * @brief Ultra-fast O(N) LSD Radix Sort for 32-bit unsigned integers.
 */
void cp_algo_radix_sort_u32(u32 *arr, usize len);

/* -------------------------------------------------------------------------
 * Searching Algorithms
 * ------------------------------------------------------------------------- */
isize cp_algo_bsearch_i32(const i32 *arr, usize len, i32 key);
isize cp_algo_bsearch_u32(const u32 *arr, usize len, u32 key);

/* -------------------------------------------------------------------------
 * Utility Algorithms
 * ------------------------------------------------------------------------- */
void cp_algo_shuffle(void *base, usize num, usize size);
void cp_algo_reverse(void *base, usize num, usize size);

/* -------------------------------------------------------------------------
 * Fast Lock-Free Pseudo-Random Number Generator (Xoshiro256**)
 * ------------------------------------------------------------------------- */
typedef struct {
    u64 s[4];
} cp_rand_t;

typedef cp_rand_t Rng;

void cp_rand_init(cp_rand_t *rng, u64 seed);
u64 cp_rand_u64(cp_rand_t *rng);
f64 cp_rand_f64(cp_rand_t *rng);
u64 cp_rand_range(cp_rand_t *rng, u64 min, u64 max);

void cp_rand_seed(u64 seed);
u64 cp_rand_next_u64(void);
f64 cp_rand_next_f64(void);
u64 cp_rand_next_range(u64 min, u64 max);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define AlgoSortI32(arr, len)          cp_algo_sort_i32(arr, len)
#define AlgoSortU32(arr, len)          cp_algo_sort_u32(arr, len)
#define AlgoSortF32(arr, len)          cp_algo_sort_f32(arr, len)
#define AlgoSort(base, num, sz, cmp)   cp_algo_sort(base, num, sz, cmp)
#define AlgoRadixSortU32(arr, len)     cp_algo_radix_sort_u32(arr, len)

#define AlgoBinarySearchI32(a, l, k)   cp_algo_bsearch_i32(a, l, k)
#define AlgoBinarySearchU32(a, l, k)   cp_algo_bsearch_u32(a, l, k)

#define AlgoShuffle(base, num, size)   cp_algo_shuffle(base, num, size)
#define AlgoReverse(base, num, size)   cp_algo_reverse(base, num, size)

#define RandInit(rng, seed)            cp_rand_init(rng, seed)
#define RandU64(rng)                   cp_rand_u64(rng)
#define RandF64(rng)                   cp_rand_f64(rng)
#define RandRange(rng, min, max)       cp_rand_range(rng, min, max)
#define RandSeed(seed)                 cp_rand_seed(seed)
#define RandNextU64()                  cp_rand_next_u64()
#define RandNextF64()                  cp_rand_next_f64()
#define RandNextRange(min, max)        cp_rand_next_range(min, max)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*const sortI32)(i32 *arr, usize len);
    void (*const sortU32)(u32 *arr, usize len);
    void (*const sortF32)(f32 *arr, usize len);
    void (*const sort)(void *base, usize num, usize size, int (*cmp)(const void *, const void *));
    void (*const radixSortU32)(u32 *arr, usize len);
    isize (*const bsearchI32)(const i32 *arr, usize len, i32 key);
    isize (*const bsearchU32)(const u32 *arr, usize len, u32 key);
    void (*const shuffle)(void *base, usize num, usize size);
    void (*const reverse)(void *base, usize num, usize size);
} cp_algo_namespace_t;

static const cp_algo_namespace_t algo = {
    .sortI32 = cp_algo_sort_i32,
    .sortU32 = cp_algo_sort_u32,
    .sortF32 = cp_algo_sort_f32,
    .sort = cp_algo_sort,
    .radixSortU32 = cp_algo_radix_sort_u32,
    .bsearchI32 = cp_algo_bsearch_i32,
    .bsearchU32 = cp_algo_bsearch_u32,
    .shuffle = cp_algo_shuffle,
    .reverse = cp_algo_reverse,
};

typedef struct {
    void (*const init)(cp_rand_t *rng, u64 seed);
    u64 (*const u64_)(cp_rand_t *rng);
    f64 (*const f64_)(cp_rand_t *rng);
    u64 (*const range)(cp_rand_t *rng, u64 min, u64 max);
    void (*const seed)(u64 seed);
    u64 (*const nextU64)(void);
    f64 (*const nextF64)(void);
    u64 (*const nextRange)(u64 min, u64 max);
} cp_rng_namespace_t;

static const cp_rng_namespace_t rng = {
    .init = cp_rand_init,
    .u64_ = cp_rand_u64,
    .f64_ = cp_rand_f64,
    .range = cp_rand_range,
    .seed = cp_rand_seed,
    .nextU64 = cp_rand_next_u64,
    .nextF64 = cp_rand_next_f64,
    .nextRange = cp_rand_next_range,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_ALGO_H */
