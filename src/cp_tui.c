/**
 * @file cp_tui.c
 * @brief High-performance, zero-dependency double-buffered Terminal UI (TUI) engine.
 */

#include "cplus/cp_tui.h"

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
