/**
 * @file test_str_fs.c
 * @brief Unit tests for cp_str and cp_fs modules.
 */

#include "cplus/cplus.h"
#include <assert.h>
#include <string.h>

static void test_string_view_operations(void) {
    printf("[TEST] StringView operations (PascalCase & Dot-Notation)...\n");

    StrView s = S("  Hello, Modern Cplus World!  ");
    assert(s.len == 30);

    /* Trim operations */
    StrView trimmed = StrTrim(s);
    assert(trimmed.len == 26);
    assert(StrEqualsC(trimmed, "Hello, Modern Cplus World!"));

    /* Substring */
    StrView sub1 = StrSub(trimmed, 0, 5);
    assert(StrEqualsC(sub1, "Hello"));
    CP_UNUSED(sub1);

    StrView sub2 = str.sub(trimmed, 7, 12);
    assert(str.equals(sub2, S("Modern Cplus")));
    CP_UNUSED(sub2);

    /* Prefix / Suffix */
    assert(StrStartsWith(trimmed, S("Hello")));
    assert(!StrStartsWith(trimmed, S("World")));
    assert(str.endsWith(trimmed, S("World!")));

    /* Substring search */
    isize pos = StrFind(trimmed, S("Cplus"));
    assert(pos == 14);
    CP_UNUSED(pos);

    isize not_found = str.find(trimmed, S("Python"));
    assert(not_found == -1);
    CP_UNUSED(not_found);

    /* Splitting */
    StrView csv = S("alpha,beta,gamma,delta");
    StrView items[8];
    usize count = StrSplit(csv, ',', items, 8);
    assert(count == 4);
    assert(StrEqualsC(items[0], "alpha"));
    assert(StrEqualsC(items[1], "beta"));
    assert(StrEqualsC(items[2], "gamma"));
    assert(StrEqualsC(items[3], "delta"));
    CP_UNUSED(count);

    /* Arena split */
    cp_arena_t *arena = ArenaCreate(4096);
    usize arena_count = 0;
    StrView *arena_items = str.split(csv, ',', arena, &arena_count);
    assert(arena_count == 4);
    assert(StrEqualsC(arena_items[0], "alpha"));
    assert(StrEqualsC(arena_items[3], "delta"));
    CP_UNUSED(arena_items);

    ArenaDestroy(arena);
    printf("  -> StringView tests PASSED.\n");
}

static void test_string_builder_operations(void) {
    printf("[TEST] String Builder & Format operations...\n");

    cp_arena_t *a = ArenaCreate(4096);

    /* String creation & append */
    Str sb = StrNew(a, 16);
    StrAppendC(&sb, "Key=");
    StrAppend(&sb, S("Value123"));
    assert(sb.len == 12);
    assert(strcmp(sb.data, "Key=Value123") == 0);

    /* StrFormat */
    Str formatted = StrFormat(a, "Temperature: %d C, Status: %s", 25, "OK");
    assert(StrEqualsC(cp_str_to_sv(&formatted), "Temperature: 25 C, Status: OK"));
    CP_UNUSED(formatted);

    /* Scratch format */
    Str scratch_fmt = StrFormatScratch("Counter: %d", 100);
    assert(StrEqualsC(cp_str_to_sv(&scratch_fmt), "Counter: 100"));
    CP_UNUSED(scratch_fmt);

    /* Join */
    StrView parts[] = { S("usr"), S("local"), S("bin") };
    Str joined = str.join(a, parts, 3, S("/"));
    assert(StrEqualsC(cp_str_to_sv(&joined), "usr/local/bin"));
    CP_UNUSED(joined);

    /* Replace */
    StrView original = S("The red dog jumped over the red fox");
    Str replaced = str.replace(a, original, S("red"), S("blue"));
    assert(StrEqualsC(cp_str_to_sv(&replaced), "The blue dog jumped over the blue fox"));
    CP_UNUSED(replaced);

    ArenaDestroy(a);
    printf("  -> String Builder tests PASSED.\n");
}

static void test_filesystem_and_paths(void) {
    printf("[TEST] Filesystem & Path operations...\n");

    const char *test_file = "/tmp/cplus_test_fs.txt";
    const char *test_content = "Cplus Zero-Latency High Performance Storage\nLine 2 Data";

    /* Write */
    b32 ok = FileWrite(test_file, test_content, strlen(test_content));
    assert(ok == true);
    assert(FileExists(test_file) == true);
    assert(FileSize(test_file) == strlen(test_content));
    CP_UNUSED(ok);

    /* Read */
    cp_arena_t *a = ArenaCreate(4096);
    StrView read_back = FileRead(a, test_file);
    assert(read_back.data != NULL);
    assert(read_back.len == strlen(test_content));
    assert(StrEqualsC(read_back, test_content));
    CP_UNUSED(read_back);

    /* Append */
    const char *extra = "\nAppended Line 3";
    assert(FileAppend(test_file, extra, strlen(extra)) == true);
    assert(FileSize(test_file) == strlen(test_content) + strlen(extra));
    CP_UNUSED(extra);

    /* Dot-notation Read */
    StrView read_dot = fs.read(a, test_file);
    assert(read_dot.len == strlen(test_content) + strlen(extra));
    assert(fs.exists(test_file) == true);
    CP_UNUSED(read_dot);

    /* Test FileReadScratch */
    StrView read_scratch = FileReadScratch(test_file);
    assert(read_scratch.len == strlen(test_content) + strlen(extra));
    CP_UNUSED(read_scratch);

    /* Test FileWriteStr and FileAppendStr */
    const char *test_str_file = "/tmp/cplus_test_write_str.txt";
    assert(FileWriteStr(test_str_file, S("HeaderLine\n")) == true);
    assert(FileAppendStr(test_str_file, S("BodyLine\n")) == true);
    assert(FileExists(test_str_file) == true);
    assert(FileRemove(test_str_file) == true);
    CP_UNUSED(test_str_file);

    /* Test FileMkdir and FileIsDir */
    const char *test_dir = "/tmp/cplus_test_dir";
    assert(FileMkdir(test_dir) == true);
    assert(FileIsDir(test_dir) == true);
    assert(FileRemove(test_dir) == true);
    CP_UNUSED(test_dir);

    /* Directory listing on /tmp */
    usize count = 0;
    StrView *entries = fs.listDir(a, "/tmp", &count);
    assert(entries != NULL);
    assert(count > 0);
    CP_UNUSED(entries);

    /* Verify test_file is cleaned up */
    assert(fs.remove(test_file) == true);
    assert(fs.exists(test_file) == false);

    /* Path operations */
    StrView p1 = S("home/antonio");
    StrView p2 = S("projects/Cplus");
    StrView joined_path = PathJoin(a, p1, p2);
    assert(StrEqualsC(joined_path, "home/antonio/projects/Cplus"));
    CP_UNUSED(joined_path);

    StrView file_path = S("/var/log/system.debug.log");
    StrView ext = PathExt(file_path);
    assert(StrEqualsC(ext, ".log"));
    CP_UNUSED(ext);

    StrView base = PathBase(file_path);
    assert(StrEqualsC(base, "system.debug.log"));
    CP_UNUSED(base);

    StrView dir = PathDir(a, file_path);
    assert(StrEqualsC(dir, "/var/log"));
    CP_UNUSED(dir);

    ArenaDestroy(a);
    printf("  -> Filesystem & Path tests PASSED.\n");
}

int main(void) {
    printf("\n=== RUNNING CPLUS STRING & FILESYSTEM TESTS ===\n");
    test_string_view_operations();
    test_string_builder_operations();
    test_filesystem_and_paths();
    printf("=== ALL UNIT TESTS PASSED SUCCESSFULLY! ===\n\n");
    return 0;
}
