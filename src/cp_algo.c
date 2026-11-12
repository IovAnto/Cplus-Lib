/**
 * @file cp_algo.c
 * @brief High-performance implementations of IntroSort, RadixSort, Binary Search, and Shuffling.
 */

#include "cplus/cp_algo.h"
#include "cplus/cp_mem.h"
#include <string.h>

#define INSERTION_SORT_THRESHOLD 16

/* -------------------------------------------------------------------------
 * Insertion Sort Helpers
 * ------------------------------------------------------------------------- */
static void _insertion_sort_i32(i32 *arr, usize len) {
    for (usize i = 1; i < len; i++) {
        i32 key = arr[i];
        isize j = (isize)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

static void _insertion_sort_u32(u32 *arr, usize len) {
    for (usize i = 1; i < len; i++) {
        u32 key = arr[i];
        isize j = (isize)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

static void _insertion_sort_f32(f32 *arr, usize len) {
    for (usize i = 1; i < len; i++) {
        f32 key = arr[i];
        isize j = (isize)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/* -------------------------------------------------------------------------
 * IntroSort / QuickSort Implementations
 * ------------------------------------------------------------------------- */
static void _quicksort_i32(i32 *arr, usize len) {
    while (len > INSERTION_SORT_THRESHOLD) {
        /* Median of three pivot */
        usize mid = len / 2;
        if (arr[0] > arr[mid]) { i32 t = arr[0]; arr[0] = arr[mid]; arr[mid] = t; }
        if (arr[0] > arr[len - 1]) { i32 t = arr[0]; arr[0] = arr[len - 1]; arr[len - 1] = t; }
        if (arr[mid] > arr[len - 1]) { i32 t = arr[mid]; arr[mid] = arr[len - 1]; arr[len - 1] = t; }

        i32 pivot = arr[mid];
        isize i = -1;
        isize j = (isize)len;

        for (;;) {
            do { i++; } while (arr[i] < pivot);
            do { j--; } while (arr[j] > pivot);
            if (i >= j) break;

            i32 tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }

        /* Tail-call recursion on smaller partition */
        usize left_len = (usize)(j + 1);
        usize right_len = len - left_len;

        if (left_len < right_len) {
            _quicksort_i32(arr, left_len);
            arr += left_len;
            len = right_len;
        } else {
            _quicksort_i32(arr + left_len, right_len);
            len = left_len;
        }
    }
    _insertion_sort_i32(arr, len);
}

void cp_algo_sort_i32(i32 *arr, usize len) {
    if (!arr || len < 2) return;
    _quicksort_i32(arr, len);
}

static void _quicksort_u32(u32 *arr, usize len) {
    while (len > INSERTION_SORT_THRESHOLD) {
        usize mid = len / 2;
        if (arr[0] > arr[mid]) { u32 t = arr[0]; arr[0] = arr[mid]; arr[mid] = t; }
        if (arr[0] > arr[len - 1]) { u32 t = arr[0]; arr[0] = arr[len - 1]; arr[len - 1] = t; }
        if (arr[mid] > arr[len - 1]) { u32 t = arr[mid]; arr[mid] = arr[len - 1]; arr[len - 1] = t; }

        u32 pivot = arr[mid];
        isize i = -1;
        isize j = (isize)len;

        for (;;) {
            do { i++; } while (arr[i] < pivot);
            do { j--; } while (arr[j] > pivot);
            if (i >= j) break;

            u32 tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }

        usize left_len = (usize)(j + 1);
        usize right_len = len - left_len;

        if (left_len < right_len) {
            _quicksort_u32(arr, left_len);
            arr += left_len;
            len = right_len;
        } else {
            _quicksort_u32(arr + left_len, right_len);
            len = left_len;
        }
    }
    _insertion_sort_u32(arr, len);
}

void cp_algo_sort_u32(u32 *arr, usize len) {
    if (!arr || len < 2) return;
    _quicksort_u32(arr, len);
}

static void _quicksort_f32(f32 *arr, usize len) {
    while (len > INSERTION_SORT_THRESHOLD) {
        usize mid = len / 2;
        if (arr[0] > arr[mid]) { f32 t = arr[0]; arr[0] = arr[mid]; arr[mid] = t; }
        if (arr[0] > arr[len - 1]) { f32 t = arr[0]; arr[0] = arr[len - 1]; arr[len - 1] = t; }
        if (arr[mid] > arr[len - 1]) { f32 t = arr[mid]; arr[mid] = arr[len - 1]; arr[len - 1] = t; }

        f32 pivot = arr[mid];
        isize i = -1;
        isize j = (isize)len;

        for (;;) {
            do { i++; } while (arr[i] < pivot);
            do { j--; } while (arr[j] > pivot);
            if (i >= j) break;

            f32 tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }

        usize left_len = (usize)(j + 1);
        usize right_len = len - left_len;

        if (left_len < right_len) {
            _quicksort_f32(arr, left_len);
            arr += left_len;
            len = right_len;
        } else {
            _quicksort_f32(arr + left_len, right_len);
            len = left_len;
        }
    }
    _insertion_sort_f32(arr, len);
}

void cp_algo_sort_f32(f32 *arr, usize len) {
    if (!arr || len < 2) return;
    _quicksort_f32(arr, len);
}

void cp_algo_sort(void *base, usize num, usize size, int (*cmp)(const void *, const void *)) {
    if (!base || num < 2 || !cmp) return;
    qsort(base, num, size, cmp);
}

/* -------------------------------------------------------------------------
 * O(N) LSD Radix Sort for u32
 * ------------------------------------------------------------------------- */
void cp_algo_radix_sort_u32(u32 *arr, usize len) {
    if (!arr || len < 2) return;

    cp_temp_arena_t scratch = ScratchBegin();
    u32 *temp = (u32 *)ArenaAlloc(scratch.arena, len * sizeof(u32));

    u32 *src = arr;
    u32 *dst = temp;

    /* 4 passes of 8 bits each (256 buckets) */
    for (usize shift = 0; shift < 32; shift += 8) {
        usize count[256] = {0};

        for (usize i = 0; i < len; i++) {
            u8 byte = (u8)((src[i] >> shift) & 0xFF);
            count[byte]++;
        }

        usize offset[256];
        offset[0] = 0;
        for (usize i = 1; i < 256; i++) {
            offset[i] = offset[i - 1] + count[i - 1];
        }

        for (usize i = 0; i < len; i++) {
            u8 byte = (u8)((src[i] >> shift) & 0xFF);
            dst[offset[byte]++] = src[i];
        }

        /* Swap src and dst pointers */
        u32 *swap = src;
        src = dst;
        dst = swap;
    }

    /* If final data is in temp, copy back to arr */
    if (src != arr) {
        memcpy(arr, src, len * sizeof(u32));
    }

    ScratchEnd(scratch);
}

/* -------------------------------------------------------------------------
 * Binary Search
 * ------------------------------------------------------------------------- */
isize cp_algo_bsearch_i32(const i32 *arr, usize len, i32 key) {
    if (!arr || len == 0) return -1;
    isize low = 0;
    isize high = (isize)len - 1;

    while (low <= high) {
        isize mid = low + (high - low) / 2;
        if (arr[mid] == key) return mid;
        if (arr[mid] < key) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

isize cp_algo_bsearch_u32(const u32 *arr, usize len, u32 key) {
    if (!arr || len == 0) return -1;
    isize low = 0;
    isize high = (isize)len - 1;

    while (low <= high) {
        isize mid = low + (high - low) / 2;
        if (arr[mid] == key) return mid;
        if (arr[mid] < key) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Utility Algorithms (Shuffle & Reverse)
 * ------------------------------------------------------------------------- */
static u32 _xorshift32_state = 2463534242U;

static u32 _xorshift32(void) {
    u32 x = _xorshift32_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    _xorshift32_state = x;
    return x;
}

void cp_algo_shuffle(void *base, usize num, usize size) {
    if (!base || num < 2 || size == 0) return;
    u8 *arr = (u8 *)base;
    u8 *temp = (u8 *)alloca(size);

    for (usize i = num - 1; i > 0; i--) {
        usize j = (usize)(_xorshift32() % (u32)(i + 1));
        if (i != j) {
            memcpy(temp, arr + i * size, size);
            memcpy(arr + i * size, arr + j * size, size);
            memcpy(arr + j * size, temp, size);
        }
    }
}

void cp_algo_reverse(void *base, usize num, usize size) {
    if (!base || num < 2 || size == 0) return;
    u8 *arr = (u8 *)base;
    u8 *temp = (u8 *)alloca(size);

    for (usize i = 0; i < num / 2; i++) {
        usize j = num - 1 - i;
        memcpy(temp, arr + i * size, size);
        memcpy(arr + i * size, arr + j * size, size);
        memcpy(arr + j * size, temp, size);
    }
}

/* -------------------------------------------------------------------------
 * Fast Lock-Free Pseudo-Random Number Generator (Xoshiro256**)
 * ------------------------------------------------------------------------- */
static inline u64 _xoshiro_rotl(const u64 x, int k) {
    return (x << k) | (x >> (64 - k));
}

static u64 _splitmix64(u64 *state) {
    u64 z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void cp_rand_init(cp_rand_t *rng, u64 seed) {
    if (!rng) return;
    u64 sm_state = (seed == 0) ? 0x853c49e6748fea9bULL : seed;
    rng->s[0] = _splitmix64(&sm_state);
    rng->s[1] = _splitmix64(&sm_state);
    rng->s[2] = _splitmix64(&sm_state);
    rng->s[3] = _splitmix64(&sm_state);
}

u64 cp_rand_u64(cp_rand_t *rng) {
    if (!rng) return 0;
    const u64 result = _xoshiro_rotl(rng->s[1] * 5, 7) * 9;
    const u64 t = rng->s[1] << 17;

    rng->s[2] ^= rng->s[0];
    rng->s[3] ^= rng->s[1];
    rng->s[1] ^= rng->s[2];
    rng->s[0] ^= rng->s[3];

    rng->s[2] ^= t;
    rng->s[3] = _xoshiro_rotl(rng->s[3], 45);

    return result;
}

f64 cp_rand_f64(cp_rand_t *rng) {
    return (f64)(cp_rand_u64(rng) >> 11) * (1.0 / 9007199254740992.0);
}

u64 cp_rand_range(cp_rand_t *rng, u64 min, u64 max) {
    if (min >= max) return min;
    u64 range = max - min + 1;
    return min + (cp_rand_u64(rng) % range);
}

static cp_rand_t _global_rng = {
    .s = { 0x123456789abcdef0ULL, 0xfedcba9876543210ULL, 0x0123456789abcdefULL, 0x89abcdef01234567ULL }
};

void cp_rand_seed(u64 seed) {
    cp_rand_init(&_global_rng, seed);
}

u64 cp_rand_next_u64(void) {
    return cp_rand_u64(&_global_rng);
}

f64 cp_rand_next_f64(void) {
    return cp_rand_f64(&_global_rng);
}

u64 cp_rand_next_range(u64 min, u64 max) {
    return cp_rand_range(&_global_rng, min, max);
}
