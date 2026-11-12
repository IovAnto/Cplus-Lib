/**
 * @file demo_pure_c_monitor.c
 * @brief Industrial Telemetry Monitor implemented in PURE C (Standard C11).
 *        Shows the traditional manual approach: raw ANSI escape codes, manual cursor
 *        positioning, full-screen redraws (\033[2J), and no abstraction library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_WHITE   "\033[1;37m"
#define COLOR_GRAY    "\033[90m"

static void pure_c_gotoxy(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

/* Manual Box Drawing Function in Pure C */
static void pure_c_draw_box(int x, int y, int w, int h, const char *title, const char *color) {
    /* Top border */
    pure_c_gotoxy(x, y);
    printf("%s┌", color);
    int right = x + w - 1;
    int bottom = y + h - 1;

    int title_len = title ? (int)strlen(title) : 0;
    int inner_w = w - 2;
    int start_t = (inner_w > title_len + 4) ? (x + 2 + (inner_w - title_len - 4) / 2) : 0;

    for (int col = x + 1; col < right; col++) {
        if (start_t > 0 && col == start_t) {
            printf(" %s%s%s%s ", COLOR_WHITE, COLOR_BOLD, title, color);
            col += title_len + 3;
        } else {
            printf("─");
        }
    }
    pure_c_gotoxy(right, y);
    printf("┐%s", COLOR_RESET);

    /* Side borders */
    for (int row = y + 1; row < bottom; row++) {
        pure_c_gotoxy(x, row);
        printf("%s│%s", color, COLOR_RESET);
        for (int col = x + 1; col < right; col++) {
            putchar(' ');
        }
        pure_c_gotoxy(right, row);
        printf("%s│%s", color, COLOR_RESET);
    }

    /* Bottom border */
    pure_c_gotoxy(x, bottom);
    printf("%s└", color);
    for (int col = x + 1; col < right; col++) {
        printf("─");
    }
    printf("┘%s", COLOR_RESET);
}

/* Manual Progress Bar in Pure C */
static void pure_c_draw_progress_bar(int x, int y, int width, float pct, const char *label, const char *bar_color) {
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;

    pure_c_gotoxy(x, y);
    printf("%s%s%-9s%s [", COLOR_WHITE, COLOR_BOLD, label, COLOR_RESET);

    int bar_w = width - 18;
    int filled = (int)((float)bar_w * pct);

    printf("%s", bar_color);
    for (int i = 0; i < bar_w; i++) {
        if (i < filled) {
            printf("█");
        } else {
            printf("%s░%s", COLOR_GRAY, bar_color);
        }
    }
    printf("%s] %s%3d%%%s", COLOR_RESET, COLOR_WHITE, (int)(pct * 100.0f), COLOR_RESET);
}

static void pure_c_render_dashboard(int frame) {
    /* Traditional Pure C: Full screen clear (causes flickering in live terminal) */
    printf("\033[2J\033[H");

    /* 1. Main Outer Window */
    pure_c_draw_box(1, 1, 78, 22, "EDGE CONTROLLER TELEMETRY - NODE #01 (ONLINE)", COLOR_CYAN);

    /* Mode Banner */
    pure_c_gotoxy(3, 2);
    printf("%s[ IMPLEMENTATION: PURE C (Standard C11 / Manual ANSI Escape Codes) ]%s", COLOR_GRAY, COLOR_RESET);

    /* 2. Left Sub-box: Hardware & Sensor Telemetry */
    pure_c_draw_box(3, 4, 35, 12, "Hardware Core Sensors", COLOR_BLUE);

    float cpu_pct = 0.55f + 0.25f * sinf((float)frame * 0.15f);
    float ram_pct = 0.42f + 0.05f * cosf((float)frame * 0.10f);

    pure_c_draw_progress_bar(5, 6, 31, cpu_pct, "CPU Core", COLOR_GREEN);
    pure_c_draw_progress_bar(5, 8, 31, ram_pct, "Arena RAM", COLOR_CYAN);

    pure_c_gotoxy(5, 10);
    printf("%sTemperature:%s   48.5 °C  (Normal)", COLOR_WHITE, COLOR_RESET);
    pure_c_gotoxy(5, 11);
    printf("%sCooling Fan:%s   3,240 RPM", COLOR_WHITE, COLOR_RESET);
    pure_c_gotoxy(5, 12);
    printf("%sVoltage Rail:%s  +12.04V | +5.01V | +3.30V", COLOR_WHITE, COLOR_RESET);
    pure_c_gotoxy(5, 13);
    printf("%sThermal State:%s NOMINAL (Tdp: 15W)", COLOR_GREEN, COLOR_RESET);

    /* 3. Right Sub-box: Industrial Bus & Networking */
    pure_c_draw_box(40, 4, 37, 12, "Industrial Bus & Network", COLOR_BLUE);

    float bus_load = 0.12f + 0.04f * sinf((float)frame * 0.20f);
    pure_c_draw_progress_bar(42, 6, 33, bus_load, "CAN Load", COLOR_GREEN);

    pure_c_gotoxy(42, 8);
    printf("%sModbus RTU:%s     1,000,000 frames/s", COLOR_WHITE, COLOR_RESET);
    pure_c_gotoxy(42, 9);
    printf("%sModbus CRC16:%s   Hardware LUT (0xCDC5)", COLOR_WHITE, COLOR_RESET);
    pure_c_gotoxy(42, 10);
    printf("%sSocketCAN:%s      vcan0 (98.15 M fps)", COLOR_WHITE, COLOR_RESET);
    pure_c_gotoxy(42, 11);
    printf("%sThread Pool:%s    4/4 Pre-Warmed Workers", COLOR_WHITE, COLOR_RESET);
    pure_c_gotoxy(42, 12);
    printf("%sDispatch Rate:%s  1,497,800 tasks/s", COLOR_GREEN, COLOR_RESET);
    pure_c_gotoxy(42, 13);
    printf("%sFrame Number:%s   #%04d (Naive Full Redraw)%s", COLOR_YELLOW, COLOR_RESET, frame, COLOR_RESET);

    /* 4. Bottom Sub-box: Event & Diagnostic Log */
    pure_c_draw_box(3, 17, 74, 5, "System Event Log", COLOR_GREEN);

    pure_c_gotoxy(5, 19);
    printf("%s[INFO]%s 11:45:01 Watchdog timer refreshed. Subsystems responsive.", COLOR_CYAN, COLOR_RESET);
    pure_c_gotoxy(5, 20);
    printf("%s[WARN]%s Full redraw active: sending 190+ KB/s over terminal stdout.", COLOR_YELLOW, COLOR_RESET);

    pure_c_gotoxy(1, 23);
    fflush(stdout);
}

int main(int argc, char **argv) {
    bool is_static = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--static") == 0 || strcmp(argv[i], "-s") == 0) {
            is_static = true;
        }
    }

    if (is_static || !isatty(STDOUT_FILENO)) {
        pure_c_render_dashboard(1);
        printf("\n");
        return 0;
    }

    /* Live animation mode: Notice visible flicker due to \033[2J and full-screen stdout blast */
    for (int f = 0; f < 45; f++) {
        pure_c_render_dashboard(f);
        usleep(40000); /* ~25 FPS */
    }

    printf("\n[Pure C Demo] Finished rendering 45 frames.\n");
    return 0;
}
