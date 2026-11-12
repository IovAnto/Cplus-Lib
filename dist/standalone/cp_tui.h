/**
 * @file cp_tui.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_tui.
 *
 * In ONE file define:
 *   #define CPLUS_CP_TUI_IMPLEMENTATION
 *   #include "cp_tui.h"
 */
#ifndef CPLUS_CP_TUI_H
#define CPLUS_CP_TUI_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
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
 * @file cp_tui.h
 * @brief High-performance, zero-dependency double-buffered Terminal UI (TUI) engine.
 *        Features sub-microsecond differential ANSI updates, Unicode boxes, progress bars,
 *        and crash-safe terminal restoration.
 */


#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Color Definitions (ANSI 16-color palette)
 * ------------------------------------------------------------------------- */
#define TUI_COLOR_DEFAULT        0
#define TUI_COLOR_BLACK          1
#define TUI_COLOR_RED            2
#define TUI_COLOR_GREEN          3
#define TUI_COLOR_YELLOW         4
#define TUI_COLOR_BLUE           5
#define TUI_COLOR_MAGENTA        6
#define TUI_COLOR_CYAN           7
#define TUI_COLOR_WHITE          8
#define TUI_COLOR_BRIGHT_BLACK   9
#define TUI_COLOR_BRIGHT_RED     10
#define TUI_COLOR_BRIGHT_GREEN   11
#define TUI_COLOR_BRIGHT_YELLOW  12
#define TUI_COLOR_BRIGHT_BLUE    13
#define TUI_COLOR_BRIGHT_MAGENTA 14
#define TUI_COLOR_BRIGHT_CYAN    15
#define TUI_COLOR_BRIGHT_WHITE   16

/* -------------------------------------------------------------------------
 * Text Attributes
 * ------------------------------------------------------------------------- */
#define TUI_ATTR_NONE            0x00
#define TUI_ATTR_BOLD            0x01
#define TUI_ATTR_DIM             0x02
#define TUI_ATTR_UNDERLINE       0x04
#define TUI_ATTR_REVERSE         0x08

/* -------------------------------------------------------------------------
 * Input Key Codes
 * ------------------------------------------------------------------------- */
typedef enum {
    TUI_KEY_NONE      = 0,
    TUI_KEY_UP        = 1000,
    TUI_KEY_DOWN,
    TUI_KEY_LEFT,
    TUI_KEY_RIGHT,
    TUI_KEY_ENTER,
    TUI_KEY_ESCAPE,
    TUI_KEY_BACKSPACE,
    TUI_KEY_TAB
} cp_tui_key_t;

typedef cp_tui_key_t TuiKey;

/* -------------------------------------------------------------------------
 * Cell & TUI Matrix Structures
 * ------------------------------------------------------------------------- */
typedef struct {
    char glyph[8]; /**< UTF-8 multi-byte or ASCII glyph (null-terminated) */
    u8 fg;         /**< Foreground color */
    u8 bg;         /**< Background color */
    u8 attrs;      /**< Attributes bitmask */
    u8 _pad;
} cp_tui_cell_t;

typedef cp_tui_cell_t TuiCell;

typedef struct cp_tui {
    usize width;
    usize height;
    cp_tui_cell_t *front_buffer;
    cp_tui_cell_t *back_buffer;
    char *out_buffer;
    usize out_cap;
    usize out_len;
    bool is_headless;
    bool raw_mode_active;
    struct termios orig_termios;
} cp_tui_t;

typedef cp_tui_t Tui;

/* -------------------------------------------------------------------------
 * Lifecycle Management
 * ------------------------------------------------------------------------- */
/**
 * @brief Initialize TUI on physical terminal.
 * @param width Desired width (0 = auto-detect terminal size).
 * @param height Desired height (0 = auto-detect terminal size).
 */
cp_tui_t *cp_tui_init(usize width, usize height);

/**
 * @brief Create a headless TUI engine for off-screen rendering and automated tests.
 */
cp_tui_t *cp_tui_create(usize width, usize height, bool is_headless);

/**
 * @brief Shutdown TUI, restore terminal mode and cursor, and free memory.
 */
void cp_tui_shutdown(cp_tui_t *tui);

/* -------------------------------------------------------------------------
 * Rendering & Differential Flusher
 * ------------------------------------------------------------------------- */
/**
 * @brief Clear the back buffer with default or specified background color.
 */
void cp_tui_clear(cp_tui_t *tui, u8 bg);

/**
 * @brief Compare back buffer with front buffer and flush only changed cells.
 * @return Number of characters written to the output stream.
 */
usize cp_tui_present(cp_tui_t *tui);

/* -------------------------------------------------------------------------
 * Drawing Primitives
 * ------------------------------------------------------------------------- */
void cp_tui_set_cell(cp_tui_t *tui, usize x, usize y, const char *glyph, u8 fg, u8 bg, u8 attrs);
void cp_tui_print(cp_tui_t *tui, usize x, usize y, const char *str, u8 fg, u8 bg, u8 attrs);
void cp_tui_box(cp_tui_t *tui, usize x, usize y, usize w, usize h, const char *title, u8 fg, u8 bg);
void cp_tui_progress_bar(cp_tui_t *tui, usize x, usize y, usize width, f32 percentage, const char *label, u8 fg, u8 bg);

/* -------------------------------------------------------------------------
 * Interactive Components & Input
 * ------------------------------------------------------------------------- */
cp_tui_key_t cp_tui_read_key(cp_tui_t *tui, u32 timeout_ms);
bool cp_tui_menu(cp_tui_t *tui, usize x, usize y, usize w, const char *title, const char **options, usize count, usize *selected_index);
bool cp_tui_prompt(cp_tui_t *tui, usize x, usize y, usize w, const char *prompt, char *out_buf, usize cap);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define TuiInit(w, h)                            cp_tui_init(w, h)
#define TuiCreate(w, h, headless)                cp_tui_create(w, h, headless)
#define TuiShutdown(tui)                         cp_tui_shutdown(tui)
#define TuiClear(tui, bg)                        cp_tui_clear(tui, bg)
#define TuiPresent(tui)                          cp_tui_present(tui)
#define TuiSetCell(tui, x, y, g, fg, bg, a)      cp_tui_set_cell(tui, x, y, g, fg, bg, a)
#define TuiPrint(tui, x, y, str, fg, bg, a)      cp_tui_print(tui, x, y, str, fg, bg, a)
#define TuiBox(tui, x, y, w, h, t, fg, bg)       cp_tui_box(tui, x, y, w, h, t, fg, bg)
#define TuiProgressBar(tui, x, y, w, p, l, fg, bg) cp_tui_progress_bar(tui, x, y, w, p, l, fg, bg)
#define TuiReadKey(tui, ms)                      cp_tui_read_key(tui, ms)
#define TuiMenu(tui, x, y, w, t, opt, cnt, idx)  cp_tui_menu(tui, x, y, w, t, opt, cnt, idx)
#define TuiPrompt(tui, x, y, w, p, buf, cap)     cp_tui_prompt(tui, x, y, w, p, buf, cap)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_tui_t *(*const init)(usize width, usize height);
    cp_tui_t *(*const create)(usize width, usize height, bool is_headless);
    void (*const shutdown)(cp_tui_t *tui);
    void (*const clear)(cp_tui_t *tui, u8 bg);
    usize (*const present)(cp_tui_t *tui);
    void (*const setCell)(cp_tui_t *tui, usize x, usize y, const char *glyph, u8 fg, u8 bg, u8 attrs);
    void (*const print)(cp_tui_t *tui, usize x, usize y, const char *str, u8 fg, u8 bg, u8 attrs);
    void (*const box)(cp_tui_t *tui, usize x, usize y, usize w, usize h, const char *title, u8 fg, u8 bg);
    void (*const progressBar)(cp_tui_t *tui, usize x, usize y, usize width, f32 percentage, const char *label, u8 fg, u8 bg);
    cp_tui_key_t (*const readKey)(cp_tui_t *tui, u32 timeout_ms);
    bool (*const menu)(cp_tui_t *tui, usize x, usize y, usize w, const char *title, const char **options, usize count, usize *selected_index);
    bool (*const prompt)(cp_tui_t *tui, usize x, usize y, usize w, const char *prompt, char *out_buf, usize cap);
} cp_tui_namespace_t;

static const cp_tui_namespace_t tui = {
    .init = cp_tui_init,
    .create = cp_tui_create,
    .shutdown = cp_tui_shutdown,
    .clear = cp_tui_clear,
    .present = cp_tui_present,
    .setCell = cp_tui_set_cell,
    .print = cp_tui_print,
    .box = cp_tui_box,
    .progressBar = cp_tui_progress_bar,
    .readKey = cp_tui_read_key,
    .menu = cp_tui_menu,
    .prompt = cp_tui_prompt,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_TUI_H */

#ifdef CPLUS_CP_TUI_IMPLEMENTATION
/**
 * @file cp_tui.c
 * @brief High-performance, zero-dependency double-buffered Terminal UI (TUI) engine.
 */


#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <poll.h>

/* -------------------------------------------------------------------------
 * Global Crash-Safety State
 * ------------------------------------------------------------------------- */
static cp_tui_t *g_active_tui = NULL;

static void cp_tui_signal_handler(int sig) {
    if (g_active_tui) {
        cp_tui_shutdown(g_active_tui);
    }
    _exit(128 + sig);
}

/* -------------------------------------------------------------------------
 * Internal Output Buffer Helpers
 * ------------------------------------------------------------------------- */
static void cp_tui_out_write(cp_tui_t *tui, const char *str, usize len) {
    if (tui->out_len + len >= tui->out_cap) {
        usize new_cap = (tui->out_cap + len) * 2;
        char *nb = (char *)realloc(tui->out_buffer, new_cap);
        if (!nb) return;
        tui->out_buffer = nb;
        tui->out_cap = new_cap;
    }
    memcpy(tui->out_buffer + tui->out_len, str, len);
    tui->out_len += len;
}

static void cp_tui_out_printf(cp_tui_t *tui, const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > 0) {
        cp_tui_out_write(tui, buf, (usize)n);
    }
}

static void cp_tui_emit_sgr(cp_tui_t *tui, u8 fg, u8 bg, u8 attrs) {
    cp_tui_out_write(tui, "\033[0", 3);

    if (attrs & TUI_ATTR_BOLD)      cp_tui_out_write(tui, ";1", 2);
    if (attrs & TUI_ATTR_DIM)       cp_tui_out_write(tui, ";2", 2);
    if (attrs & TUI_ATTR_UNDERLINE) cp_tui_out_write(tui, ";4", 2);
    if (attrs & TUI_ATTR_REVERSE)   cp_tui_out_write(tui, ";7", 2);

    /* Foreground */
    if (fg >= 1 && fg <= 8) {
        cp_tui_out_printf(tui, ";%d", 29 + fg);
    } else if (fg >= 9 && fg <= 16) {
        cp_tui_out_printf(tui, ";%d", 81 + fg);
    }

    /* Background */
    if (bg >= 1 && bg <= 8) {
        cp_tui_out_printf(tui, ";%d", 39 + bg);
    } else if (bg >= 9 && bg <= 16) {
        cp_tui_out_printf(tui, ";%d", 91 + bg);
    }

    cp_tui_out_write(tui, "m", 1);
}

/* -------------------------------------------------------------------------
 * Lifecycle Management
 * ------------------------------------------------------------------------- */
cp_tui_t *cp_tui_create(usize width, usize height, bool is_headless) {
    if ((width == 0 || height == 0) && !is_headless) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
            width = (usize)ws.ws_col;
            height = (usize)ws.ws_row;
        }
    }
    if (width == 0)  width = 80;
    if (height == 0) height = 24;

    cp_tui_t *tui = (cp_tui_t *)calloc(1, sizeof(cp_tui_t));
    if (!tui) return NULL;

    tui->width = width;
    tui->height = height;
    tui->is_headless = is_headless;

    usize cell_count = width * height;
    tui->front_buffer = (cp_tui_cell_t *)calloc(cell_count, sizeof(cp_tui_cell_t));
    tui->back_buffer = (cp_tui_cell_t *)calloc(cell_count, sizeof(cp_tui_cell_t));

    tui->out_cap = cell_count * 16;
    if (tui->out_cap < 8192) tui->out_cap = 8192;
    tui->out_buffer = (char *)malloc(tui->out_cap);
    tui->out_len = 0;

    if (!tui->front_buffer || !tui->back_buffer || !tui->out_buffer) {
        cp_tui_shutdown(tui);
        return NULL;
    }

    /* Initialize front buffer with empty glyphs so initial present renders cleanly */
    for (usize i = 0; i < cell_count; ++i) {
        tui->front_buffer[i].glyph[0] = '\0';
        tui->front_buffer[i].fg = 255;
        tui->front_buffer[i].bg = 255;
        tui->front_buffer[i].attrs = 255;

        tui->back_buffer[i].glyph[0] = ' ';
        tui->back_buffer[i].glyph[1] = '\0';
    }

    if (!is_headless && isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
        if (tcgetattr(STDIN_FILENO, &tui->orig_termios) == 0) {
            struct termios raw = tui->orig_termios;
            raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
            raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
            raw.c_cflag |= (CS8);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            tui->raw_mode_active = true;

            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = cp_tui_signal_handler;
            sigaction(SIGINT, &sa, NULL);
            sigaction(SIGTERM, &sa, NULL);

            /* Switch to alternate screen and hide cursor */
            ssize_t w = write(STDOUT_FILENO, "\033[?1049h\033[?25l", 14);
            CP_UNUSED(w);

            g_active_tui = tui;
        }
    }

    return tui;
}

cp_tui_t *cp_tui_init(usize width, usize height) {
    return cp_tui_create(width, height, false);
}

void cp_tui_shutdown(cp_tui_t *tui) {
    if (!tui) return;

    if (tui->raw_mode_active) {
        /* Show cursor and exit alternate screen */
        ssize_t w = write(STDOUT_FILENO, "\033[?25h\033[?1049l", 14);
        CP_UNUSED(w);
        tcsetattr(STDIN_FILENO, TCSANOW, &tui->orig_termios);
        tui->raw_mode_active = false;
    }

    if (g_active_tui == tui) {
        g_active_tui = NULL;
    }

    free(tui->front_buffer);
    free(tui->back_buffer);
    free(tui->out_buffer);
    free(tui);
}

/* -------------------------------------------------------------------------
 * Rendering & Differential Flusher
 * ------------------------------------------------------------------------- */
void cp_tui_clear(cp_tui_t *tui, u8 bg) {
    if (!tui) return;
    usize count = tui->width * tui->height;
    for (usize i = 0; i < count; ++i) {
        tui->back_buffer[i].glyph[0] = ' ';
        tui->back_buffer[i].glyph[1] = '\0';
        tui->back_buffer[i].fg = TUI_COLOR_DEFAULT;
        tui->back_buffer[i].bg = bg;
        tui->back_buffer[i].attrs = TUI_ATTR_NONE;
    }
}

usize cp_tui_present(cp_tui_t *tui) {
    if (!tui) return 0;
    tui->out_len = 0;

    u8 cur_fg = 255;
    u8 cur_bg = 255;
    u8 cur_attrs = 255;
    usize cur_x = (usize)-1;
    usize cur_y = (usize)-1;

    for (usize y = 0; y < tui->height; ++y) {
        for (usize x = 0; x < tui->width; ++x) {
            usize idx = y * tui->width + x;
            cp_tui_cell_t *b = &tui->back_buffer[idx];
            cp_tui_cell_t *f = &tui->front_buffer[idx];

            if (memcmp(b, f, sizeof(cp_tui_cell_t)) != 0) {
                /* Cursor position changed */
                if (cur_y != y || cur_x != x) {
                    cp_tui_out_printf(tui, "\033[%zu;%zuH", y + 1, x + 1);
                    cur_x = x;
                    cur_y = y;
                }

                /* SGR attribute changed */
                if (b->fg != cur_fg || b->bg != cur_bg || b->attrs != cur_attrs) {
                    cp_tui_emit_sgr(tui, b->fg, b->bg, b->attrs);
                    cur_fg = b->fg;
                    cur_bg = b->bg;
                    cur_attrs = b->attrs;
                }

                /* Write glyph */
                const char *g = (b->glyph[0] != '\0') ? b->glyph : " ";
                usize glen = strlen(g);
                cp_tui_out_write(tui, g, glen);

                cur_x++;
                *f = *b;
            }
        }
    }

    if (cur_fg != 255 || cur_bg != 255 || cur_attrs != 255) {
        cp_tui_out_write(tui, "\033[0m", 4);
    }

    if (!tui->is_headless && tui->out_len > 0) {
        ssize_t w = write(STDOUT_FILENO, tui->out_buffer, tui->out_len);
        CP_UNUSED(w);
    }

    return tui->out_len;
}

/* -------------------------------------------------------------------------
 * Drawing Primitives
 * ------------------------------------------------------------------------- */
void cp_tui_set_cell(cp_tui_t *tui, usize x, usize y, const char *glyph, u8 fg, u8 bg, u8 attrs) {
    if (!tui || x >= tui->width || y >= tui->height) return;
    usize idx = y * tui->width + x;
    cp_tui_cell_t *cell = &tui->back_buffer[idx];

    cell->fg = fg;
    cell->bg = bg;
    cell->attrs = attrs;
    memset(cell->glyph, 0, sizeof(cell->glyph));

    if (glyph && glyph[0] != '\0') {
        usize glen = strlen(glyph);
        if (glen >= sizeof(cell->glyph)) glen = sizeof(cell->glyph) - 1;
        memcpy(cell->glyph, glyph, glen);
        cell->glyph[glen] = '\0';
    } else {
        cell->glyph[0] = ' ';
        cell->glyph[1] = '\0';
    }
}

void cp_tui_print(cp_tui_t *tui, usize x, usize y, const char *str, u8 fg, u8 bg, u8 attrs) {
    if (!tui || !str || y >= tui->height) return;

    usize cur_x = x;
    const char *p = str;

    while (*p && cur_x < tui->width) {
        char glyph[8];
        memset(glyph, 0, sizeof(glyph));

        unsigned char c = (unsigned char)*p;
        usize char_len = 1;
        if ((c & 0x80) == 0) {
            char_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        }

        for (usize i = 0; i < char_len && *p; ++i) {
            glyph[i] = *p++;
        }

        cp_tui_set_cell(tui, cur_x, y, glyph, fg, bg, attrs);
        cur_x++;
    }
}

void cp_tui_box(cp_tui_t *tui, usize x, usize y, usize w, usize h, const char *title, u8 fg, u8 bg) {
    if (!tui || w < 2 || h < 2 || x >= tui->width || y >= tui->height) return;

    usize right = (x + w <= tui->width) ? (x + w - 1) : (tui->width - 1);
    usize bottom = (y + h <= tui->height) ? (y + h - 1) : (tui->height - 1);

    /* Unicode Box Characters */
    const char *TOP_LEFT     = "\xe2\x94\x8c"; /* ┌ */
    const char *TOP_RIGHT    = "\xe2\x94\x90"; /* ┐ */
    const char *BOTTOM_LEFT  = "\xe2\x94\x94"; /* └ */
    const char *BOTTOM_RIGHT = "\xe2\x94\x98"; /* ┘ */
    const char *HORIZONTAL   = "\xe2\x94\x80"; /* ─ */
    const char *VERTICAL     = "\xe2\x94\x82"; /* │ */

    /* Corners */
    cp_tui_set_cell(tui, x, y, TOP_LEFT, fg, bg, TUI_ATTR_NONE);
    cp_tui_set_cell(tui, right, y, TOP_RIGHT, fg, bg, TUI_ATTR_NONE);
    cp_tui_set_cell(tui, x, bottom, BOTTOM_LEFT, fg, bg, TUI_ATTR_NONE);
    cp_tui_set_cell(tui, right, bottom, BOTTOM_RIGHT, fg, bg, TUI_ATTR_NONE);

    /* Horizontal lines */
    for (usize col = x + 1; col < right; ++col) {
        cp_tui_set_cell(tui, col, y, HORIZONTAL, fg, bg, TUI_ATTR_NONE);
        cp_tui_set_cell(tui, col, bottom, HORIZONTAL, fg, bg, TUI_ATTR_NONE);
    }

    /* Vertical lines & interior clear */
    for (usize row = y + 1; row < bottom; ++row) {
        cp_tui_set_cell(tui, x, row, VERTICAL, fg, bg, TUI_ATTR_NONE);
        cp_tui_set_cell(tui, right, row, VERTICAL, fg, bg, TUI_ATTR_NONE);
        for (usize col = x + 1; col < right; ++col) {
            cp_tui_set_cell(tui, col, row, " ", fg, bg, TUI_ATTR_NONE);
        }
    }

    /* Centered Title */
    if (title && title[0] != '\0') {
        usize title_len = strlen(title);
        usize inner_w = right - x - 1;
        if (title_len + 4 <= inner_w) {
            usize start_col = x + 2 + (inner_w - title_len - 4) / 2;
            cp_tui_set_cell(tui, start_col, y, " ", fg, bg, TUI_ATTR_NONE);
            cp_tui_print(tui, start_col + 1, y, title, fg, bg, TUI_ATTR_BOLD);
            cp_tui_set_cell(tui, start_col + 1 + title_len, y, " ", fg, bg, TUI_ATTR_NONE);
        }
    }
}

void cp_tui_progress_bar(cp_tui_t *tui, usize x, usize y, usize width, f32 percentage, const char *label, u8 fg, u8 bg) {
    if (!tui || width < 10 || x >= tui->width || y >= tui->height) return;

    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 1.0f) percentage = 1.0f;

    char pct_str[16];
    int pct_val = (int)(percentage * 100.0f + 0.5f);
    snprintf(pct_str, sizeof(pct_str), " %3d%%", pct_val);
    usize pct_len = strlen(pct_str);

    usize label_len = label ? strlen(label) : 0;
    usize prefix_w = (label_len > 0) ? (label_len + 1) : 0;

    if (prefix_w > 0) {
        cp_tui_print(tui, x, y, label, fg, bg, TUI_ATTR_BOLD);
        cp_tui_set_cell(tui, x + label_len, y, " ", fg, bg, TUI_ATTR_NONE);
    }

    usize bar_start = x + prefix_w;
    usize bar_w = (width > (prefix_w + pct_len + 2)) ? (width - prefix_w - pct_len - 2) : 4;

    cp_tui_set_cell(tui, bar_start, y, "[", fg, bg, TUI_ATTR_NONE);

    usize filled_units = (usize)((f32)bar_w * percentage);
    const char *FILLED = "\xe2\x96\x88"; /* █ */
    const char *EMPTY  = "\xe2\x96\x91"; /* ░ */

    for (usize i = 0; i < bar_w; ++i) {
        if (i < filled_units) {
            cp_tui_set_cell(tui, bar_start + 1 + i, y, FILLED, TUI_COLOR_BRIGHT_GREEN, bg, TUI_ATTR_BOLD);
        } else {
            cp_tui_set_cell(tui, bar_start + 1 + i, y, EMPTY, TUI_COLOR_BRIGHT_BLACK, bg, TUI_ATTR_DIM);
        }
    }

    cp_tui_set_cell(tui, bar_start + 1 + bar_w, y, "]", fg, bg, TUI_ATTR_NONE);
    cp_tui_print(tui, bar_start + 2 + bar_w, y, pct_str, fg, bg, TUI_ATTR_BOLD);
}

/* -------------------------------------------------------------------------
 * Input & Interactive Components
 * ------------------------------------------------------------------------- */
cp_tui_key_t cp_tui_read_key(cp_tui_t *tui, u32 timeout_ms) {
    if (!tui || tui->is_headless) return TUI_KEY_NONE;

    struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN, .revents = 0 };
    int r = poll(&pfd, 1, (int)timeout_ms);
    if (r <= 0) return TUI_KEY_NONE;

    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return TUI_KEY_NONE;

    if (c == 27) { /* Escape sequence */
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return TUI_KEY_ESCAPE;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return TUI_KEY_ESCAPE;

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return TUI_KEY_UP;
                case 'B': return TUI_KEY_DOWN;
                case 'C': return TUI_KEY_RIGHT;
                case 'D': return TUI_KEY_LEFT;
                default: break;
            }
        }
        return TUI_KEY_ESCAPE;
    }

    if (c == '\n' || c == '\r') return TUI_KEY_ENTER;
    if (c == 127 || c == 8)     return TUI_KEY_BACKSPACE;
    if (c == '\t')              return TUI_KEY_TAB;

    return (cp_tui_key_t)(unsigned char)c;
}

bool cp_tui_menu(cp_tui_t *tui, usize x, usize y, usize w, const char *title, const char **options, usize count, usize *selected_index) {
    if (!tui || !options || count == 0 || !selected_index) return false;

    usize h = count + 2;
    usize curr = (*selected_index < count) ? *selected_index : 0;

    while (1) {
        cp_tui_box(tui, x, y, w, h, title, TUI_COLOR_BRIGHT_BLUE, TUI_COLOR_DEFAULT);

        for (usize i = 0; i < count; ++i) {
            char line[256];
            bool is_selected = (i == curr);
            snprintf(line, sizeof(line), " %c %-.*s", is_selected ? '>' : ' ', (int)(w - 6), options[i]);
            u8 fg = is_selected ? TUI_COLOR_BRIGHT_WHITE : TUI_COLOR_DEFAULT;
            u8 bg = is_selected ? TUI_COLOR_BLUE : TUI_COLOR_DEFAULT;
            u8 attr = is_selected ? (TUI_ATTR_BOLD | TUI_ATTR_REVERSE) : TUI_ATTR_NONE;
            cp_tui_print(tui, x + 1, y + 1 + i, line, fg, bg, attr);
        }

        cp_tui_present(tui);

        if (tui->is_headless) {
            *selected_index = curr;
            return true;
        }

        cp_tui_key_t key = cp_tui_read_key(tui, 100);
        if (key == TUI_KEY_UP || key == (cp_tui_key_t)'k') {
            curr = (curr == 0) ? (count - 1) : (curr - 1);
        } else if (key == TUI_KEY_DOWN || key == (cp_tui_key_t)'j') {
            curr = (curr + 1) % count;
        } else if (key == TUI_KEY_ENTER) {
            *selected_index = curr;
            return true;
        } else if (key == TUI_KEY_ESCAPE || key == (cp_tui_key_t)'q') {
            return false;
        }
    }
}

bool cp_tui_prompt(cp_tui_t *tui, usize x, usize y, usize w, const char *prompt, char *out_buf, usize cap) {
    if (!tui || !out_buf || cap == 0) return false;

    usize len = strlen(out_buf);
    usize h = 3;

    while (1) {
        cp_tui_box(tui, x, y, w, h, prompt, TUI_COLOR_BRIGHT_CYAN, TUI_COLOR_DEFAULT);

        /* Draw current text with cursor */
        char display[256];
        snprintf(display, sizeof(display), " %-.*s_", (int)(w - 5), out_buf);
        cp_tui_print(tui, x + 1, y + 1, display, TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_BOLD);

        cp_tui_present(tui);

        if (tui->is_headless) return true;

        cp_tui_key_t key = cp_tui_read_key(tui, 100);
        if (key == TUI_KEY_ENTER) {
            return true;
        } else if (key == TUI_KEY_ESCAPE) {
            return false;
        } else if (key == TUI_KEY_BACKSPACE) {
            if (len > 0) {
                out_buf[--len] = '\0';
            }
        } else if (key >= 32 && key <= 126 && len + 1 < cap) {
            out_buf[len++] = (char)key;
            out_buf[len] = '\0';
        }
    }
}
#endif /* CPLUS_CP_TUI_IMPLEMENTATION */
