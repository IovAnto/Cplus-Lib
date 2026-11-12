/**
 * @file cp_fs.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_fs.
 *
 * In ONE file define:
 *   #define CPLUS_CP_FS_IMPLEMENTATION
 *   #include "cp_fs.h"
 */
#ifndef CPLUS_CP_FS_H
#define CPLUS_CP_FS_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Core types dependency */
#ifndef CPLUS_CORE_H
#define CPLUS_CORE_H
/**
 * @file cp_core.h
 * @brief Foundational types, platform macros, RAII defer, and polymorphic printing.
 */


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <alloca.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Exact-width & ergonomic type aliases
 * ------------------------------------------------------------------------- */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef float     f32;
typedef double    f64;

typedef uint8_t   b8;
typedef uint32_t  b32;

/* -------------------------------------------------------------------------
 * Compiler & Optimization Attributes
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) || defined(__clang__)
    #define CP_INLINE static inline __attribute__((always_inline))
    #define CP_NOINLINE __attribute__((noinline))
    #define CP_LIKELY(x) __builtin_expect(!!(x), 1)
    #define CP_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define CP_ALIGN(n) __attribute__((aligned(n)))
    #define CP_UNUSED(x) ((void)(x))
#else
    #define CP_INLINE static inline
    #define CP_NOINLINE
    #define CP_LIKELY(x) (x)
    #define CP_UNLIKELY(x) (x)
    #define CP_ALIGN(n)
    #define CP_UNUSED(x) ((void)(x))
#endif

#define CP_CACHELINE_SIZE 64
#define CP_CACHELINE CP_ALIGN(CP_CACHELINE_SIZE)

/* Default SIMD alignment */
#define CP_DEFAULT_ALIGNMENT 16
#define CP_SIMD_ALIGNMENT    32

/* Helper macro token pasting */
#define CP_CONCAT_INTERNAL_(a, b) a ## b
#define CP_CONCAT(a, b) CP_CONCAT_INTERNAL_(a, b)

/* -------------------------------------------------------------------------
 * RAII Defer mechanism
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*fn)(void *);
    void *arg;
} cp_defer_t;

CP_INLINE void _cp_defer_cleanup(cp_defer_t *d) {
    if (d->fn) {
        d->fn(d->arg);
    }
}

typedef void (*cp_action_fn)(void);
CP_INLINE void _cp_defer_action_cleanup(cp_action_fn *fn) {
    if (*fn) {
        (*fn)();
    }
}

/**
 * @brief Defer execution of a function with 1 argument until the enclosing scope exits.
 * @param func_ Pointer to function taking 1 pointer argument: void func(void *arg)
 * @param arg_ Pointer passed to func when exiting scope.
 */
#define Defer(func_, arg_) \
    __attribute__((cleanup(_cp_defer_cleanup))) cp_defer_t CP_CONCAT(_cp_defer_, __LINE__) = { \
        .fn = (void (*)(void *))(func_), \
        .arg = (void *)(arg_) \
    }

/**
 * @brief Defer execution of a parameterless function until the enclosing scope exits.
 */
#define DeferAction(func_) \
    __attribute__((cleanup(_cp_defer_action_cleanup))) cp_action_fn CP_CONCAT(_cp_defact_, __LINE__) = (func_)

/* -------------------------------------------------------------------------
 * Anonymous Lambda Functions (Zero Runtime Overhead)
 * Supported natively on GCC via GNU C extension ({ ret_type fn args body fn; }).
 * On Clang / MSVC without nested function syntax, use static functions or
 * zero-overhead iteration macros (VecEach, VecMap).
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) && !defined(__clang__) && !defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__)
#define CP_HAS_LAMBDA 1
/**
 * @brief Define an anonymous inlined lambda function with zero runtime overhead (GCC).
 * @example AlgoSort(arr, len, sizeof(int), Lambda(int, (const void *a, const void *b), { return *(int*)a - *(int*)b; }));
 */
#define Lambda(ret_type, args, body) __extension__ ({ ret_type _cp_lambda args body _cp_lambda; })
#else
#define CP_HAS_LAMBDA 0
#endif

/* -------------------------------------------------------------------------
 * Polymorphic Type-Safe Printing (C11 _Generic)
 * ------------------------------------------------------------------------- */
CP_INLINE void _cp_print_bool(bool v) { fputs(v ? "true" : "false", stdout); }
CP_INLINE void _cp_print_char(char v) { putchar(v); }
CP_INLINE void _cp_print_i64(long long v) { printf("%lld", v); }
CP_INLINE void _cp_print_u64(unsigned long long v) { printf("%llu", v); }
CP_INLINE void _cp_print_f64(double v) { printf("%.6g", v); }
CP_INLINE void _cp_print_str(const char *v) { fputs(v ? v : "(null)", stdout); }
CP_INLINE void _cp_print_ptr(const void *v) { printf("%p", v); }

#define Print(val) _Generic((val), \
    _Bool: _cp_print_bool, \
    char: _cp_print_char, \
    signed char: _cp_print_i64, \
    short: _cp_print_i64, \
    int: _cp_print_i64, \
    long: _cp_print_i64, \
    long long: _cp_print_i64, \
    unsigned char: _cp_print_u64, \
    unsigned short: _cp_print_u64, \
    unsigned int: _cp_print_u64, \
    unsigned long: _cp_print_u64, \
    unsigned long long: _cp_print_u64, \
    float: _cp_print_f64, \
    double: _cp_print_f64, \
    char*: _cp_print_str, \
    const char*: _cp_print_str, \
    default: _cp_print_ptr \
)(val)

#define Println(val) do { Print(val); putchar('\n'); } while(0)
#define PrintF(fmt, ...) printf(fmt, __VA_ARGS__)

/* -------------------------------------------------------------------------
 * Panic & Assertions
 * ------------------------------------------------------------------------- */
#define Panic(msg) do { \
    fprintf(stderr, "\n[CPLUS PANIC] %s:%d: %s\n", __FILE__, __LINE__, msg); \
    abort(); \
} while(0)

#define AssertMsg(cond, msg) do { \
    if (CP_UNLIKELY(!(cond))) { \
        fprintf(stderr, "\n[CPLUS ASSERTION FAILED] %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, msg); \
        abort(); \
    } \
} while(0)

/* -------------------------------------------------------------------------
 * Bit Manipulation Intrinsics
 * ------------------------------------------------------------------------- */
CP_INLINE u32 cp_bit_popcount32(u32 x) { return (u32)__builtin_popcount(x); }
CP_INLINE u32 cp_bit_popcount64(u64 x) { return (u32)__builtin_popcountll(x); }
CP_INLINE u32 cp_bit_clz32(u32 x) { return x == 0 ? 32 : (u32)__builtin_clz(x); }
CP_INLINE u32 cp_bit_clz64(u64 x) { return x == 0 ? 64 : (u32)__builtin_clzll(x); }
CP_INLINE u32 cp_bit_ctz32(u32 x) { return x == 0 ? 32 : (u32)__builtin_ctz(x); }
CP_INLINE u32 cp_bit_ctz64(u64 x) { return x == 0 ? 64 : (u32)__builtin_ctzll(x); }
CP_INLINE u16 cp_bit_bswap16(u16 x) { return __builtin_bswap16(x); }
CP_INLINE u32 cp_bit_bswap32(u32 x) { return __builtin_bswap32(x); }
CP_INLINE u64 cp_bit_bswap64(u64 x) { return __builtin_bswap64(x); }
CP_INLINE u32 cp_bit_rotl32(u32 x, u32 s) { return (x << (s & 31)) | (x >> ((32 - (s & 31)) & 31)); }
CP_INLINE u32 cp_bit_rotr32(u32 x, u32 s) { return (x >> (s & 31)) | (x << ((32 - (s & 31)) & 31)); }
CP_INLINE u64 cp_bit_rotl64(u64 x, u32 s) { return (x << (s & 63)) | (x >> ((64 - (s & 63)) & 63)); }
CP_INLINE u64 cp_bit_rotr64(u64 x, u32 s) { return (x >> (s & 63)) | (x << ((64 - (s & 63)) & 63)); }
CP_INLINE bool cp_bit_is_pow2(u64 x) { return (x != 0) && ((x & (x - 1)) == 0); }
CP_INLINE u64 cp_bit_next_pow2(u64 x) {
    if (x <= 1) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

#define BitPopcount32(x) cp_bit_popcount32(x)
#define BitPopcount64(x) cp_bit_popcount64(x)
#define BitClz32(x)      cp_bit_clz32(x)
#define BitClz64(x)      cp_bit_clz64(x)
#define BitCtz32(x)      cp_bit_ctz32(x)
#define BitCtz64(x)      cp_bit_ctz64(x)
#define BitBswap16(x)    cp_bit_bswap16(x)
#define BitBswap32(x)    cp_bit_bswap32(x)
#define BitBswap64(x)    cp_bit_bswap64(x)
#define BitRotl32(x, s)  cp_bit_rotl32(x, s)
#define BitRotr32(x, s)  cp_bit_rotr32(x, s)
#define BitRotl64(x, s)  cp_bit_rotl64(x, s)
#define BitRotr64(x, s)  cp_bit_rotr64(x, s)
#define BitIsPow2(x)     cp_bit_is_pow2(x)
#define BitNextPow2(x)   cp_bit_next_pow2(x)

typedef struct {
    u32 (*const popcount32)(u32 x);
    u32 (*const popcount64)(u64 x);
    u32 (*const clz32)(u32 x);
    u32 (*const clz64)(u64 x);
    u32 (*const ctz32)(u32 x);
    u32 (*const ctz64)(u64 x);
    u16 (*const bswap16)(u16 x);
    u32 (*const bswap32)(u32 x);
    u64 (*const bswap64)(u64 x);
    u32 (*const rotl32)(u32 x, u32 s);
    u32 (*const rotr32)(u32 x, u32 s);
    u64 (*const rotl64)(u64 x, u32 s);
    u64 (*const rotr64)(u64 x, u32 s);
    bool (*const isPow2)(u64 x);
    u64 (*const nextPow2)(u64 x);
} cp_bit_namespace_t;

static const cp_bit_namespace_t bit = {
    .popcount32 = cp_bit_popcount32,
    .popcount64 = cp_bit_popcount64,
    .clz32 = cp_bit_clz32,
    .clz64 = cp_bit_clz64,
    .ctz32 = cp_bit_ctz32,
    .ctz64 = cp_bit_ctz64,
    .bswap16 = cp_bit_bswap16,
    .bswap32 = cp_bit_bswap32,
    .bswap64 = cp_bit_bswap64,
    .rotl32 = cp_bit_rotl32,
    .rotr32 = cp_bit_rotr32,
    .rotl64 = cp_bit_rotl64,
    .rotr64 = cp_bit_rotr64,
    .isPow2 = cp_bit_is_pow2,
    .nextPow2 = cp_bit_next_pow2,
};

/* -------------------------------------------------------------------------
 * Colored Diagnostic Logging
 * ------------------------------------------------------------------------- */
typedef enum {
    CP_LOG_LEVEL_DEBUG = 0,
    CP_LOG_LEVEL_INFO  = 1,
    CP_LOG_LEVEL_WARN  = 2,
    CP_LOG_LEVEL_ERROR = 3,
    CP_LOG_LEVEL_OFF   = 4,
} cp_log_level_t;

static inline cp_log_level_t *_cp_log_level_ptr(void) {
    static cp_log_level_t lvl = CP_LOG_LEVEL_INFO;
    return &lvl;
}

CP_INLINE void cp_log_set_level(cp_log_level_t lvl) {
    *_cp_log_level_ptr() = lvl;
}

CP_INLINE cp_log_level_t cp_log_get_level(void) {
    return *_cp_log_level_ptr();
}

#define LogSetLevel(lvl) cp_log_set_level(lvl)

#define LogDebug(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_DEBUG) { \
        fprintf(stderr, "\033[90m[DEBUG]\033[0m " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogInfo(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_INFO) { \
        fprintf(stdout, "\033[32m[INFO]\033[0m  " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogWarn(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_WARN) { \
        fprintf(stderr, "\033[33m[WARN]\033[0m  " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogError(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_ERROR) { \
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif
#endif /* CPLUS_CORE_H */

/**
 * @file cp_fs.h
 * @brief High-level, zero-latency Filesystem and Path operations.
 */



#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Filesystem Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_fs_read(cp_arena_t *arena, const char *path);
b32 cp_fs_write(const char *path, const void *data, usize len);
b32 cp_fs_append(const char *path, const void *data, usize len);

b32 cp_fs_exists(const char *path);
b32 cp_fs_is_dir(const char *path);
usize cp_fs_size(const char *path);
b32 cp_fs_remove(const char *path);
b32 cp_fs_mkdir(const char *path);

cp_strview_t *cp_fs_list_dir(cp_arena_t *arena, const char *path, usize *out_count);

/* -------------------------------------------------------------------------
 * Path Manipulation Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_path_join(cp_arena_t *arena, cp_strview_t a, cp_strview_t b);
cp_strview_t cp_path_ext(cp_strview_t path);
cp_strview_t cp_path_base(cp_strview_t path);
cp_strview_t cp_path_dir(cp_arena_t *arena, cp_strview_t path);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define FileRead(arena, path)          cp_fs_read(arena, path)
#define FileReadScratch(path)          cp_fs_read(Scratch(), path)
#define FileWrite(path, data, len)     cp_fs_write(path, data, len)
#define FileWriteStr(path, strview)    cp_fs_write(path, (strview).data, (strview).len)
#define FileAppend(path, data, len)    cp_fs_append(path, data, len)
#define FileAppendStr(path, strview)   cp_fs_append(path, (strview).data, (strview).len)

#define FileExists(path)               cp_fs_exists(path)
#define FileIsDir(path)                cp_fs_is_dir(path)
#define FileSize(path)                 cp_fs_size(path)
#define FileRemove(path)               cp_fs_remove(path)
#define FileMkdir(path)                cp_fs_mkdir(path)
#define FileListDir(arena, path, count) cp_fs_list_dir(arena, path, count)

#define PathJoin(arena, a, b)          cp_path_join(arena, a, b)
#define PathExt(path)                  cp_path_ext(path)
#define PathBase(path)                 cp_path_base(path)
#define PathDir(arena, path)           cp_path_dir(arena, path)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_strview_t (*const read)(cp_arena_t *arena, const char *path);
    b32 (*const write)(const char *path, const void *data, usize len);
    b32 (*const append)(const char *path, const void *data, usize len);
    b32 (*const exists)(const char *path);
    b32 (*const isDir)(const char *path);
    usize (*const size)(const char *path);
    b32 (*const remove)(const char *path);
    b32 (*const mkdir)(const char *path);
    cp_strview_t *(*const listDir)(cp_arena_t *arena, const char *path, usize *out_count);
} cp_fs_namespace_t;

typedef struct {
    cp_strview_t (*const join)(cp_arena_t *arena, cp_strview_t a, cp_strview_t b);
    cp_strview_t (*const ext)(cp_strview_t path);
    cp_strview_t (*const base)(cp_strview_t path);
    cp_strview_t (*const dir)(cp_arena_t *arena, cp_strview_t path);
} cp_path_namespace_t;

static const cp_fs_namespace_t fs = {
    .read = cp_fs_read,
    .write = cp_fs_write,
    .append = cp_fs_append,
    .exists = cp_fs_exists,
    .isDir = cp_fs_is_dir,
    .size = cp_fs_size,
    .remove = cp_fs_remove,
    .mkdir = cp_fs_mkdir,
    .listDir = cp_fs_list_dir,
};

static const cp_path_namespace_t path = {
    .join = cp_path_join,
    .ext = cp_path_ext,
    .base = cp_path_base,
    .dir = cp_path_dir,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_FS_H */

#ifdef CPLUS_CP_FS_IMPLEMENTATION
/**
 * @file cp_fs.c
 * @brief High-performance implementation of Filesystem and Path operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

/* -------------------------------------------------------------------------
 * Filesystem Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_fs_read(cp_arena_t *arena, const char *path) {
    if (!path) return (cp_strview_t){ NULL, 0 };
    if (!arena) arena = Scratch();

    FILE *f = fopen(path, "rb");
    if (!f) return (cp_strview_t){ NULL, 0 };

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)ArenaAlloc(arena, (usize)size + 1);
    if (!buf) {
        fclose(f);
        return (cp_strview_t){ NULL, 0 };
    }

    usize bytes_read = fread(buf, 1, (usize)size, f);
    fclose(f);

    buf[bytes_read] = '\0';
    return cp_sv_from_parts(buf, bytes_read);
}

b32 cp_fs_write(const char *path, const void *data, usize len) {
    if (!path) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    if (len > 0 && data) {
        usize written = fwrite(data, 1, len, f);
        fclose(f);
        return written == len;
    }

    fclose(f);
    return true;
}

b32 cp_fs_append(const char *path, const void *data, usize len) {
    if (!path) return false;
    FILE *f = fopen(path, "ab");
    if (!f) return false;

    if (len > 0 && data) {
        usize written = fwrite(data, 1, len, f);
        fclose(f);
        return written == len;
    }

    fclose(f);
    return true;
}

b32 cp_fs_exists(const char *path) {
    if (!path) return false;
    return access(path, F_OK) == 0;
}

b32 cp_fs_is_dir(const char *path) {
    if (!path) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

usize cp_fs_size(const char *path) {
    if (!path) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (usize)st.st_size;
}

b32 cp_fs_remove(const char *path) {
    if (!path) return false;
    return remove(path) == 0;
}

b32 cp_fs_mkdir(const char *path) {
    if (!path) return false;
    return mkdir(path, 0755) == 0;
}

cp_strview_t *cp_fs_list_dir(cp_arena_t *arena, const char *path, usize *out_count) {
    if (!path) {
        if (out_count) *out_count = 0;
        return NULL;
    }
    if (!arena) arena = Scratch();

    DIR *dir = opendir(path);
    if (!dir) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    /* Count items */
    usize count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        count++;
    }
    rewinddir(dir);

    cp_strview_t *results = (cp_strview_t *)ArenaAlloc(arena, count * sizeof(cp_strview_t));
    usize idx = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        usize name_len = strlen(entry->d_name);
        char *copy = (char *)ArenaAlloc(arena, name_len + 1);
        memcpy(copy, entry->d_name, name_len);
        copy[name_len] = '\0';
        results[idx++] = cp_sv_from_parts(copy, name_len);
    }
    closedir(dir);

    if (out_count) *out_count = count;
    return results;
}

/* -------------------------------------------------------------------------
 * Path Manipulation Operations
 * ------------------------------------------------------------------------- */
cp_strview_t cp_path_join(cp_arena_t *arena, cp_strview_t a, cp_strview_t b) {
    if (!arena) arena = Scratch();

    while (a.len > 0 && a.data[a.len - 1] == '/') {
        a.len--;
    }
    while (b.len > 0 && b.data[0] == '/') {
        b.data++;
        b.len--;
    }

    usize total_len = a.len + 1 + b.len;
    char *buf = (char *)ArenaAlloc(arena, total_len + 1);
    memcpy(buf, a.data, a.len);
    buf[a.len] = '/';
    memcpy(buf + a.len + 1, b.data, b.len);
    buf[total_len] = '\0';

    return cp_sv_from_parts(buf, total_len);
}

cp_strview_t cp_path_ext(cp_strview_t path) {
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '.') {
            return cp_sv_sub(path, i - 1, path.len - (i - 1));
        }
        if (path.data[i - 1] == '/') {
            break;
        }
    }
    return (cp_strview_t){ NULL, 0 };
}

cp_strview_t cp_path_base(cp_strview_t path) {
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '/') {
            return cp_sv_sub(path, i, path.len - i);
        }
    }
    return path;
}

cp_strview_t cp_path_dir(cp_arena_t *arena, cp_strview_t path) {
    if (!arena) arena = Scratch();
    for (usize i = path.len; i > 0; i--) {
        if (path.data[i - 1] == '/') {
            return cp_sv_sub(path, 0, i - 1);
        }
    }
    return cp_sv_from_c(".");
}
#endif /* CPLUS_CP_FS_IMPLEMENTATION */
