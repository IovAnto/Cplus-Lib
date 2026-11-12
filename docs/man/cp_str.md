# Module Specification: cp_str (Strings, Slices & Base64)

## 1. TL;DR
The `cp_str` module provides zero-allocation immutable string slices (`StrView`), Arena-backed dynamic string builders (`String`), and RFC 4648 Base64 encoding/decoding.

```c
#include "cplus/cplus.h"

/* Zero-Allocation String Slices */
StrView full = S("  GET /api/v1/telemetry HTTP/1.1  ");
StrView trimmed = StrTrim(full);
assert(StrStartsWith(trimmed, S("GET")));

/* Arena-Backed String Formatting */
TempArena scratch = ScratchBegin();
String s = StrFormat(scratch.arena, "Host: %s, Port: %d", "127.0.0.1", 8080);
printf("%s\n", s.data);
ScratchEnd(scratch);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Struct Layout: `StrView` (`cp_strview_t`)
```c
typedef struct cp_strview {
    const char *data;
    usize len;
} StrView;
```
- **Borrowing Semantics**: Stores an immutable pointer and byte count. Operations like `StrSub`, `StrTrim`, and `StrTrimLeft` simply increment `data` and adjust `len`, never allocating heap memory or copying bytes.
- **Literal Macro `S()`**: `S("literal")` evaluates `sizeof("literal") - 1` at compile-time, setting `len` with 0 runtime `strlen()` calls.

### 2.2 Struct Layout: `String` (`cp_string_t`)
```c
typedef struct cp_string {
    char *data;
    usize len;
    usize cap;
    cp_arena_t *arena;
} String;
```
- A mutable string builder backed by an `Arena`. Maintains explicit length while guaranteeing null-termination at `data[len] = '\0'` for seamless interop with standard C libc functions.

---

## 3. Operational Guarantees & Contracts

| Metric | StrView | String Builder | Base64 Engine |
|---|---|---|---|
| **Sub-Slice / Trim** | $O(1)$ zero-allocation | N/A | N/A |
| **Append / Format** | N/A | $O(1)$ amortized (on Arena) | N/A |
| **Encode / Decode** | N/A | N/A | $O(N)$ linear time |
| **Heap Allocations** | 0 bytes (borrows memory) | 0 heap calls (Arena-backed) | 0 heap calls (user buffer) |
| **Thread-Safety** | Safe for concurrent reads | Thread-confined | Pure reentrant functions |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Literal Slice | `S("literal")` | N/A | Compile-time StrView constructor |
| Slice from C-Str | `StrFromC(cstr)` | `str.fromC(cstr)` | Create StrView from null-terminated char* |
| Substring | `StrSub(sv, start, len)`| `str.sub(sv, start, len)` | Slice substring in O(1) |
| Trim Whitespace | `StrTrim(sv)` | `str.trim(sv)` | Strip leading and trailing whitespace |
| Equals Check | `StrEquals(a, b)` | `str.equals(a, b)` | Constant-time slice equality check |
| Starts With | `StrStartsWith(sv, pre)` | `str.startsWith(sv, pre)`| Prefix verification |
| Ends With | `StrEndsWith(sv, suf)` | `str.endsWith(sv, suf)` | Suffix verification |
| Find Substring | `StrFind(sv, needle)` | `str.find(sv, needle)` | Search needle, return byte offset (-1 if none) |
| Split on Arena | `StrSplitArena(sv, d, a, cnt)`| `str.split(sv, d, a, cnt)` | Split into StrView array on Arena |
| New String | `StrNew(arena, cap)` | `str.new(arena, cap)` | Create empty mutable string builder |
| String Length | `StrLen(str)` | `str.len(str)` | Current length of String builder |
| Append Slice | `StrAppend(str, sv)` | `str.append(str, sv)` | Append StrView to String builder |
| Append C-Str | `StrAppendC(str, cstr)` | `str.appendC(str, cstr)`| Append null-terminated string |
| Format String | `StrFormat(arena, fmt, ...)` | `str.format(...)` | vsnprintf directly into Arena memory |
| Scratch Format | `StrFormatScratch(fmt, ...)`| N/A | Temporary format on thread-local scratch |
| Base64 Encode | `Base64Encode(src, len, dst, cap)`| `str.base64Encode(...)` | RFC 4648 Base64 encoder |
| Base64 Decode | `Base64Decode(src, len, dst, cap)`| `str.base64Decode(...)` | RFC 4648 Base64 decoder |

---

## 5. Use Cases
1. **Zero-Copy HTTP/Protocol Parsing**: Tokenize request headers directly into `StrView` slices without copying header bytes.
2. **Log Formatting**: Build log lines into a thread-local scratch string using `StrFormatScratch`.
3. **Data Serialization**: Encode raw binary telemetry into Base64 for JSON transmission.

---

## 6. Complete C11 Usage Examples

### Example: URL Query String Tokenizer
```c
#include "cplus/cplus.h"

void parse_query_params(StrView query) {
    TempArena scratch = ScratchBegin();
    
    usize pair_count = 0;
    StrView *pairs = StrSplitArena(query, '&', scratch.arena, &pair_count);
    
    for (usize i = 0; i < pair_count; i++) {
        isize eq_pos = StrFindChar(pairs[i], '=');
        if (eq_pos > 0) {
            StrView key = StrSub(pairs[i], 0, (usize)eq_pos);
            StrView val = StrSub(pairs[i], (usize)eq_pos + 1, pairs[i].len - (usize)eq_pos - 1);
            printf("Param [%.*s] = [%.*s]\n", (int)key.len, key.data, (int)val.len, val.data);
        }
    }
    
    ScratchEnd(scratch);
}
```

---

## 7. Limitations & Operational Constraints
- **Null-Termination of `StrView`**: A `StrView` slice is **not** guaranteed to be null-terminated (e.g. after `StrSub` or `StrTrim`). When passing a `StrView` to `printf`, use the `%.*s` precision specifier (`printf("%.*s", (int)sv.len, sv.data)`). If you require a null-terminated string, use `String` or `StrFromParts`.
- **Base64 Buffer Sizing**: The destination buffer for `Base64Encode` must be at least `4 * ((src_len + 2) / 3) + 1` bytes to hold the encoded output and trailing null byte.
