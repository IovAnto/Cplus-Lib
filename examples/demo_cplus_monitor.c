/**
 * @file demo_cplus_monitor.c
 * @brief Industrial Telemetry Monitor implemented using the CPLUS FOUNDATION LIBRARY.
 *        Shows the modern ergonomic approach: high-level declarative widgets (TuiBox,
 *        TuiProgressBar, TuiPrint), sub-microsecond double-buffering, and zero screen tearing.
 */

#include "cplus/cplus.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static void cplus_render_dashboard(cp_tui_t *t, usize frame) {
    TuiClear(t, TUI_COLOR_DEFAULT);

    /* 1. Main Outer Window */
    TuiBox(t, 0, 0, 78, 22, "EDGE CONTROLLER TELEMETRY - NODE #01 (ONLINE)", TUI_COLOR_BRIGHT_CYAN, TUI_COLOR_DEFAULT);

    /* Mode Banner */
    TuiPrint(t, 2, 1, "[ IMPLEMENTATION: CPLUS LIBRARY (Double-Buffered L1 Differential Engine) ]", TUI_COLOR_BRIGHT_BLACK, TUI_COLOR_DEFAULT, TUI_ATTR_DIM);

    /* 2. Left Sub-box: Hardware & Sensor Telemetry */
    TuiBox(t, 2, 3, 35, 12, "Hardware Core Sensors", TUI_COLOR_BRIGHT_BLUE, TUI_COLOR_DEFAULT);

    f32 cpu_pct = 0.55f + 0.25f * sinf((f32)frame * 0.15f);
    f32 ram_pct = 0.42f + 0.05f * cosf((f32)frame * 0.10f);

    TuiProgressBar(t, 4, 5, 31, cpu_pct, "CPU Core", TUI_COLOR_BRIGHT_GREEN, TUI_COLOR_DEFAULT);
    TuiProgressBar(t, 4, 7, 31, ram_pct, "Arena RAM", TUI_COLOR_BRIGHT_CYAN, TUI_COLOR_DEFAULT);

    TuiPrint(t, 4, 9,  "Temperature:   48.5 °C  (Normal)", TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_NONE);
    TuiPrint(t, 4, 10, "Cooling Fan:   3,240 RPM", TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_NONE);
    TuiPrint(t, 4, 11, "Voltage Rail:  +12.04V | +5.01V | +3.30V", TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_NONE);
    TuiPrint(t, 4, 12, "Thermal State: NOMINAL (Tdp: 15W)", TUI_COLOR_BRIGHT_GREEN, TUI_COLOR_DEFAULT, TUI_ATTR_BOLD);

    /* 3. Right Sub-box: Industrial Bus & Networking */
    TuiBox(t, 39, 3, 37, 12, "Industrial Bus & Network", TUI_COLOR_BRIGHT_BLUE, TUI_COLOR_DEFAULT);

    f32 bus_load = 0.12f + 0.04f * sinf((f32)frame * 0.20f);
    TuiProgressBar(t, 41, 5, 33, bus_load, "CAN Load", TUI_COLOR_BRIGHT_GREEN, TUI_COLOR_DEFAULT);

    TuiPrint(t, 41, 7,  "Modbus RTU:     1,000,000 frames/s", TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_NONE);
    TuiPrint(t, 41, 8,  "Modbus CRC16:   Hardware LUT (0xCDC5)", TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_NONE);
    TuiPrint(t, 41, 9,  "SocketCAN:      vcan0 (98.15 M fps)", TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_NONE);
    TuiPrint(t, 41, 10, "Thread Pool:    4/4 Pre-Warmed Workers", TUI_COLOR_BRIGHT_WHITE, TUI_COLOR_DEFAULT, TUI_ATTR_NONE);
    TuiPrint(t, 41, 11, "Dispatch Rate:  1,497,800 tasks/s", TUI_COLOR_BRIGHT_GREEN, TUI_COLOR_DEFAULT, TUI_ATTR_BOLD);

    char frame_str[48];
    snprintf(frame_str, sizeof(frame_str), "Frame Number:   #%04zu (Diff L1 Flusher)", frame);
    TuiPrint(t, 41, 12, frame_str, TUI_COLOR_BRIGHT_YELLOW, TUI_COLOR_DEFAULT, TUI_ATTR_BOLD);

    /* 4. Bottom Sub-box: Event & Diagnostic Log */
    TuiBox(t, 2, 16, 74, 5, "System Event Log", TUI_COLOR_BRIGHT_GREEN, TUI_COLOR_DEFAULT);

    TuiPrint(t, 4, 18, "[INFO] 11:45:01 Watchdog timer refreshed. Subsystems responsive.", TUI_COLOR_BRIGHT_CYAN, TUI_COLOR_DEFAULT, TUI_ATTR_NONE);
    TuiPrint(t, 4, 19, "[PERF] 11:45:02 Differential flusher active: 93% I/O reduction, zero flicker.", TUI_COLOR_BRIGHT_GREEN, TUI_COLOR_DEFAULT, TUI_ATTR_BOLD);
}

int main(int argc, char **argv) {
    bool is_static = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--static") == 0 || strcmp(argv[i], "-s") == 0) {
            is_static = true;
        }
    }

    if (is_static || !isatty(STDOUT_FILENO)) {
        /* Headless snapshot mode: render single frame into L1 cache and output to stdout */
        cp_tui_t *t = TuiCreate(80, 24, true);
        cplus_render_dashboard(t, 1);
        TuiPresent(t);
        fwrite(t->out_buffer, 1, t->out_len, stdout);
        putchar('\n');
        TuiShutdown(t);
        return 0;
    }

    /* Live animation mode: Notice rock-solid stability and zero screen tearing */
    cp_tui_t *t = TuiInit(80, 24);
    for (usize f = 0; f < 45; f++) {
        cplus_render_dashboard(t, f);
        TuiPresent(t);
        usleep(40000); /* ~25 FPS */
    }
    TuiShutdown(t);

    printf("\n[Cplus Demo] Finished rendering 45 frames.\n");
    return 0;
}
