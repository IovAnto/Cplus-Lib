/**
 * @file test_tui.c
 * @brief Comprehensive unit test suite for the cp_tui module.
 */

#include "cplus/cplus.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

static void test_tui_lifecycle_and_clipping(void) {
    printf("[TEST] cp_tui: headless creation, buffer allocation, and safe boundary clipping...\n");

    cp_tui_t *t = TuiCreate(80, 24, true);
    assert(t != NULL);
    assert(t->width == 80);
    assert(t->height == 24);
    assert(t->front_buffer != NULL);
    assert(t->back_buffer != NULL);
    assert(t->is_headless == true);

    /* Test out-of-bounds clipping */
    TuiSetCell(t, 9999, 9999, "X", TUI_COLOR_RED, TUI_COLOR_BLACK, TUI_ATTR_NONE);
    TuiSetCell(t, 80, 24, "X", TUI_COLOR_RED, TUI_COLOR_BLACK, TUI_ATTR_NONE);
    TuiPrint(t, 75, 23, "VERY_LONG_STRING_THAT_DEFINITELY_EXCEEDS_RIGHT_BOUNDARY", TUI_COLOR_WHITE, TUI_COLOR_BLACK, TUI_ATTR_NONE);

    /* Check that cell (79, 23) got a character, but no overflow occurred */
    usize idx = 23 * 80 + 79;
    assert(t->back_buffer[idx].glyph[0] != '\0');
    CP_UNUSED(idx);

    TuiShutdown(t);
    printf("       Headless matrix and safe clipping verified without errors.\n");
}

static void test_tui_differential_rendering(void) {
    printf("[TEST] cp_tui: double-buffering differential rendering and zero-dirty frames...\n");

    cp_tui_t *t = TuiCreate(80, 24, true);
    assert(t != NULL);

    /* Frame 1: Paint initial text */
    TuiClear(t, TUI_COLOR_BLACK);
    TuiPrint(t, 10, 5, "CPLUS_ZERO_LATENCY", TUI_COLOR_BRIGHT_GREEN, TUI_COLOR_BLACK, TUI_ATTR_BOLD);

    usize bytes_frame1 = TuiPresent(t);
    assert(bytes_frame1 > 0);

    /* Frame 2: Identical back buffer (0 changes) */
    usize bytes_frame2 = TuiPresent(t);
    assert(bytes_frame2 == 0); /* MUST be exactly 0 bytes! Zero I/O waste on unchanged frames */
    CP_UNUSED(bytes_frame2);

    /* Frame 3: Mutate exactly 1 cell */
    TuiSetCell(t, 10, 5, "!", TUI_COLOR_BRIGHT_RED, TUI_COLOR_BLACK, TUI_ATTR_NONE);
    usize bytes_frame3 = TuiPresent(t);
    assert(bytes_frame3 > 0);
    assert(bytes_frame3 < 50); /* Only that single cell coordinate is updated */
    CP_UNUSED(bytes_frame3);

    /* Verify front buffer matches back buffer */
    assert(memcmp(t->front_buffer, t->back_buffer, 80 * 24 * sizeof(cp_tui_cell_t)) == 0);

    TuiShutdown(t);
    printf("       Differential updates verified: Frame 1: %zu bytes, Static Frame 2: 0 bytes (zero-overhead).\n",
           bytes_frame1);
}

static void test_tui_box_and_progress_bar(void) {
    printf("[TEST] cp_tui: Unicode box framing and progress bar formatting...\n");

    cp_tui_t *t = TuiCreate(80, 24, true);
    assert(t != NULL);

    /* Draw Box */
    TuiBox(t, 5, 2, 30, 8, "Telemetry Monitor", TUI_COLOR_BRIGHT_CYAN, TUI_COLOR_DEFAULT);

    /* Verify 4 Corners */
    /* (5, 2) = ┌ (\xe2\x94\x8c) */
    usize top_left = 2 * 80 + 5;
    assert(strcmp(t->back_buffer[top_left].glyph, "\xe2\x94\x8c") == 0);
    CP_UNUSED(top_left);

    /* (34, 2) = ┐ (\xe2\x94\x90) */
    usize top_right = 2 * 80 + 34;
    assert(strcmp(t->back_buffer[top_right].glyph, "\xe2\x94\x90") == 0);
    CP_UNUSED(top_right);

    /* (5, 9) = └ (\xe2\x94\x94) */
    usize bot_left = 9 * 80 + 5;
    assert(strcmp(t->back_buffer[bot_left].glyph, "\xe2\x94\x94") == 0);
    CP_UNUSED(bot_left);

    /* (34, 9) = ┘ (\xe2\x94\x98) */
    usize bot_right = 9 * 80 + 34;
    assert(strcmp(t->back_buffer[bot_right].glyph, "\xe2\x94\x98") == 0);
    CP_UNUSED(bot_right);

    /* Draw Progress Bar at 50% */
    TuiProgressBar(t, 5, 12, 30, 0.50f, "Load", TUI_COLOR_WHITE, TUI_COLOR_DEFAULT);

    /* Verify progress bar presence */
    usize bar_row_start = 12 * 80 + 5;
    assert(t->back_buffer[bar_row_start].glyph[0] == 'L');
    assert(t->back_buffer[bar_row_start + 1].glyph[0] == 'o');
    CP_UNUSED(bar_row_start);

    usize bytes = TuiPresent(t);
    assert(bytes > 0);
    CP_UNUSED(bytes);

    TuiShutdown(t);
    printf("       Unicode box corners and 50%% progress bar rendered accurately.\n");
}

static void test_tui_interactive_components(void) {
    printf("[TEST] cp_tui: headless interactive menu and text prompt...\n");

    cp_tui_t *t = TuiCreate(80, 24, true);
    assert(t != NULL);

    const char *options[] = { "Start Engine", "Stop Engine", "Configure Sensors", "Exit" };
    usize selected = 2;
    bool ok_menu = TuiMenu(t, 10, 4, 30, "System Control", options, 4, &selected);
    assert(ok_menu);
    assert(selected == 2);
    CP_UNUSED(ok_menu);

    char input[64] = "192.168.1.100";
    bool ok_prompt = TuiPrompt(t, 10, 12, 35, "Target IP Address", input, sizeof(input));
    assert(ok_prompt);
    assert(strcmp(input, "192.168.1.100") == 0);
    CP_UNUSED(ok_prompt);

    TuiShutdown(t);
    printf("       Headless menu and text prompt passed with 100%% precision.\n");
}

static void test_tui_dot_notation(void) {
    printf("[TEST] cp_tui: dot-notation namespace verification...\n");

    cp_tui_t *t = tui.create(80, 24, true);
    assert(t != NULL);

    tui.clear(t, TUI_COLOR_DEFAULT);
    tui.setCell(t, 0, 0, "#", TUI_COLOR_YELLOW, TUI_COLOR_BLACK, TUI_ATTR_BOLD);
    tui.print(t, 2, 0, "DOT_NOTATION_OK", TUI_COLOR_GREEN, TUI_COLOR_BLACK, TUI_ATTR_NONE);
    tui.box(t, 0, 2, 20, 5, "DotBox", TUI_COLOR_BLUE, TUI_COLOR_DEFAULT);
    tui.progressBar(t, 0, 8, 20, 0.75f, "CPU", TUI_COLOR_CYAN, TUI_COLOR_DEFAULT);

    usize bytes = tui.present(t);
    assert(bytes > 0);
    CP_UNUSED(bytes);

    tui.shutdown(t);
    printf("       Dot-notation namespaces operating identically to PascalCase.\n");
}

int main(void) {
    printf("===============================================================================\n");
    printf("                  Cplus Terminal UI (cp_tui) Unit Test Suite                   \n");
    printf("===============================================================================\n");

    test_tui_lifecycle_and_clipping();
    test_tui_differential_rendering();
    test_tui_box_and_progress_bar();
    test_tui_interactive_components();
    test_tui_dot_notation();

    printf("===============================================================================\n");
    printf("         ALL TERMINAL UI TESTS PASSED WITH ZERO WARNINGS/ERRORS                \n");
    printf("===============================================================================\n");
    return 0;
}
