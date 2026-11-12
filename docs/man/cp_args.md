# Module Specification: cp_args (Command-Line Argument Parser Subsystem)

## 1. TL;DR
The `cp_args` subsystem provides a lightweight, zero-allocation command-line argument parser supporting POSIX short flags (`-v`), long options (`--port 8080` or `--port=8080`), and positional arguments without dynamic memory overhead.

```c
#include "cplus/cplus.h"

int main(int argc, char **argv) {
    cp_args_t cli = ArgsParse(argc, argv);

    bool verbose = ArgsHasFlag(&cli, "-v", "--verbose");
    i64 port = ArgsGetInt(&cli, "-p", "--port", 8080);
    const char *config = ArgsGetStr(&cli, "-c", "--config", "default.json");

    if (ArgsPositionalCount(&cli) > 0) {
        const char *input_file = ArgsGetPositional(&cli, 0);
        printf("Processing input: %s\n", input_file);
    }
    return 0;
}
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Struct Layout
```c
#define CP_ARGS_MAX_POSITIONALS 64
#define CP_ARGS_MAX_OPTIONS     64
#define CP_ARGS_MAX_FLAGS       64

typedef struct {
    const char *key;
    const char *value;
} cp_arg_kv_t;

typedef struct {
    int argc;
    char **argv;
    const char *program_name;
    const char *positionals[CP_ARGS_MAX_POSITIONALS];
    usize positional_count;
    cp_arg_kv_t options[CP_ARGS_MAX_OPTIONS];
    usize option_count;
    const char *flags[CP_ARGS_MAX_FLAGS];
    usize flag_count;
} cp_args_t;
```

### 2.2 Parsing Semantics
- **Zero Allocations**: All string pointers in `cp_args_t` directly reference tokens in `argv` or string literals. No memory is allocated on the heap.
- **Short & Long Syntax**:
  - Flags: Matches boolean switches like `-v`, `--verbose`, `-h`, `--help`.
  - Key-Value Pairs: Supports both separate arguments (`--host 127.0.0.1`, `-p 8080`) and inline equals syntax (`--host=127.0.0.1`, `-p=8080`).
- **Positional Isolation**: Tokens not starting with `-` (or appearing after a double dash `--`) are collected sequentially into the `positionals` table.

---

## 3. Operational Guarantees & Contracts

| Metric | `ArgsParse` | Flag Check | Option Lookup | Positional Lookup |
|---|---|---|---|---|
| **Time Complexity** | $O(N)$ arguments | $O(F)$ flag count | $O(O)$ option count | $O(1)$ direct array index |
| **Heap Allocations** | 0 | 0 | 0 | 0 |
| **Memory Footprint** | Fixed stack struct (~2 KB) | 0 | 0 | 0 |
| **Thread Safety** | Reentrant | Read-only thread-safe | Read-only thread-safe | Read-only thread-safe |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Parse Arguments | `ArgsParse(argc, argv)` | `args.parse(argc, argv)` | Parse `argc`/`argv` into a populated `cp_args_t` struct |
| Check Flag | `ArgsHasFlag(a, sf, lf)` | `args.hasFlag(a, sf, lf)` | Check if short or long boolean flag is present |
| Get String Option | `ArgsGetStr(a, sf, lf, def)` | `args.getStr(a, sf, lf, def)` | Extract string value by short/long switch with fallback |
| Get Integer Option | `ArgsGetInt(a, sf, lf, def)` | `args.getInt(a, sf, lf, def)` | Extract integer value by short/long switch with fallback |
| Get Float Option | `ArgsGetFloat(a, sf, lf, def)` | `args.getFloat(a, sf, lf, def)` | Extract floating-point value by short/long switch with fallback |
| Get Positional | `ArgsGetPositional(a, idx)` | `args.getPositional(a, idx)` | Retrieve positional argument at 0-based index |
| Positional Count | `ArgsPositionalCount(a)` | `args.positionalCount(a)` | Total number of non-option arguments parsed |

---

## 5. Use Cases
1. **Server CLI Flags**: Configure listening port (`-p 8080`), bind address (`-b 0.0.0.0`), and daemon mode (`-d`).
2. **Build Tools & Compilers**: Parse input source files positionally alongside include paths (`-I include`) and optimization levels (`-O3`).
3. **Data Migrators**: Accept dry-run switches (`--dry-run`), batch sizes (`--batch-size 1000`), and target database URLs.

---

## 6. Complete Compilable Example

```c
#include "cplus/cplus.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    /* Simulated CLI invocation: app --port 9090 -v --timeout=3.5 /var/log/app.log */
    char *mock_argv[] = {
        "app",
        "--port", "9090",
        "-v",
        "--timeout=3.5",
        "/var/log/app.log",
        NULL
    };
    int mock_argc = 5;

    cp_args_t parsed = args.parse(mock_argc, mock_argv);

    assert(args.hasFlag(&parsed, "-v", "--verbose") == true);
    assert(args.hasFlag(&parsed, "-d", "--debug") == false);

    i64 port = args.getInt(&parsed, "-p", "--port", 8080);
    assert(port == 9090);

    f64 timeout = args.getFloat(&parsed, "-t", "--timeout", 1.0);
    assert(timeout > 3.49 && timeout < 3.51);

    assert(args.positionalCount(&parsed) == 1);
    const char *target = args.getPositional(&parsed, 0);
    assert(strcmp(target, "/var/log/app.log") == 0);

    printf("CLI Configuration Verified: Port=%lld, Timeout=%.1f, Positional='%s'\n",
           port, timeout, target);

    return 0;
}
```

---

## 7. Limitations & Edge Cases
- **Fixed Capacity Limits**: Supports up to 64 positionals, 64 options, and 64 flags. In CLI tools with hundreds of flags, increase `#define CP_ARGS_MAX_*` constants before including the header.
- **Combined Short Flags**: Clustered short flags (e.g. `-xzvf`) are treated as individual single token keys rather than separated into four individual flags. Separate them into `-x -z -v -f` or check the full clustered string.
