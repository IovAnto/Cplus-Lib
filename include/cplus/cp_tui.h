/**
 * @file cp_tui.h
 * @brief High-performance, zero-dependency double-buffered Terminal UI (TUI) engine.
 *        Features sub-microsecond differential ANSI updates, Unicode boxes, progress bars,
 *        and crash-safe terminal restoration.
 */

#ifndef CPLUS_TUI_H
#define CPLUS_TUI_H

#include "cp_core.h"
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

#endif /* CPLUS_TUI_H */
