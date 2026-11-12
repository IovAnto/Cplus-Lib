# Module Specification: cp_json (Micro JSON Parser Subsystem)

## 1. TL;DR
The `cp_json` subsystem is an in-situ, zero-allocation micro JSON parser compliant with RFC 8259. It navigates JSON payloads by returning `StrView` non-owning string slices without performing heap allocations or mutating the input buffer.

```c
#include "cplus/cplus.h"

const char *payload = "{\"status\":200,\"success\":true,\"nodes\":[\"srv-1\",\"srv-2\"]}";
cp_json_val_t root = JsonParse(payload, strlen(payload));

i64 status = JsonAsInt(JsonGet(root, "status"), 0);           // 200
bool ok = JsonAsBool(JsonGet(root, "success"), false);        // true

cp_json_val_t nodes = JsonGet(root, "nodes");
usize count = JsonLen(nodes);                                 // 2
StrView first_node = JsonAsView(JsonAt(nodes, 0));            // "srv-1"
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Types and Data Structures
```c
typedef enum {
    CP_JSON_INVALID = 0,
    CP_JSON_NULL,
    CP_JSON_BOOL,
    CP_JSON_NUMBER,
    CP_JSON_STRING,
    CP_JSON_ARRAY,
    CP_JSON_OBJECT
} cp_json_type_t;

typedef struct {
    StrView raw;
    cp_json_type_t type;
} cp_json_val_t;
```

### 2.2 Memory Model & Traversal
- **Zero Allocations**: `cp_json_val_t` references a slice of the original buffer via `StrView` (`const char *data`, `usize len`). No memory is allocated on the heap or arena during parsing.
- **In-Situ Tokenization**: `cp_json_parse` validates syntactic boundaries (braces, brackets, quotes, numbers) and detects root element types in a single linear pass.
- **Key & Index Traversal**:
  - `JsonGet(obj, key)` scans keys within JSON object `{ ... }`, skipping nested container levels by tracking structural balance counters `{}`, `[]`, and quotes.
  - `JsonAt(arr, index)` steps through array elements `[ ... ]` using structural delimiter commas `,` until the target index is reached.
- **Conversion Primitives**:
  - `JsonAsView`: Strips enclosing quote delimiters for strings or returns raw numeric/literal slices.
  - `JsonAsInt` / `JsonAsFloat`: Parses integer or floating point values directly from the underlying byte slice.
  - `JsonAsBool`: Checks slice against literal `"true"`.

---

## 3. Operational Guarantees & Contracts

| Operation | Time Complexity | Auxiliary Memory | Heap Allocations | Input Mutation |
|---|---|---|---|---|
| `JsonParse` | $O(1)$ root detection | 0 bytes | 0 | None (read-only) |
| `JsonGet` | $O(N)$ key scan | 0 bytes | 0 | None (read-only) |
| `JsonAt` | $O(K)$ index scan | 0 bytes | 0 | None (read-only) |
| `JsonLen` | $O(N)$ container scan | 0 bytes | 0 | None (read-only) |
| `JsonAsInt` / `Float` | $O(D)$ digit count | 0 bytes | 0 | None (read-only) |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Parse JSON | `JsonParse(str, len)` | `json_api.parse(str, len)` | Parse JSON buffer into root `cp_json_val_t` |
| Get Key Value | `JsonGet(obj, key)` | `json_api.get(obj, key)` | Query member of JSON object by key name |
| Get Array Item | `JsonAt(arr, index)` | `json_api.at(arr, index)` | Retrieve element at 0-based index from array |
| Container Length | `JsonLen(val)` | `json_api.len(val)` | Count elements in JSON object or array |
| Unquote View | `JsonAsView(val)` | `json_api.asView(val)` | Return unquoted `StrView` string slice |
| Read Integer | `JsonAsInt(val, def)` | `json_api.asInt(val, def)` | Convert numeric node to `i64` with fallback |
| Read Float | `JsonAsFloat(val, def)` | `json_api.asFloat(val, def)` | Convert numeric node to `f64` with fallback |
| Read Boolean | `JsonAsBool(val, def)` | `json_api.asBool(val, def)` | Convert boolean node to `bool` with fallback |
| Check Null | `JsonIsNull(val)` | `json_api.isNull(val)` | Check if node represents JSON `null` |

---

## 5. Use Cases
1. **REST API Response Inspection**: Extract configuration parameters, bearer tokens, or error status codes directly from HTTP response payloads.
2. **Embedded Config Files**: Read application configuration schemas in JSON format without dragging in bloated multi-megabyte C++ parsing libraries.
3. **Telemetry & Log Querying**: Fast-path inspection of structured JSON log records over high-bandwidth TCP streams.

---

## 6. Complete Compilable Example

```c
#include "cplus/cplus.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    const char *payload = 
        "{\n"
        "  \"host\": \"api.internal.net\",\n"
        "  \"port\": 8443,\n"
        "  \"tls\": true,\n"
        "  \"replicas\": [\"node-a\", \"node-b\", \"node-c\"]\n"
        "}";

    cp_json_val_t root = json_api.parse(payload, strlen(payload));
    assert(root.type == CP_JSON_OBJECT);

    /* Query object fields */
    cp_json_val_t host_val = json_api.get(root, "host");
    StrView host_sv = json_api.asView(host_val);
    assert(StrViewEqStr(host_sv, "api.internal.net"));

    i64 port = json_api.asInt(json_api.get(root, "port"), 80);
    assert(port == 8443);

    bool tls = json_api.asBool(json_api.get(root, "tls"), false);
    assert(tls == true);

    /* Query array */
    cp_json_val_t replicas = json_api.get(root, "replicas");
    assert(replicas.type == CP_JSON_ARRAY);
    assert(json_api.len(replicas) == 3);

    StrView first_replica = json_api.asView(json_api.at(replicas, 0));
    assert(StrViewEqStr(first_replica, "node-a"));

    printf("Parsed configuration for %.*s on port %lld (TLS: %d)\n",
           (int)host_sv.len, host_sv.data, port, tls);

    return 0;
}
```

---

## 7. Limitations & Edge Cases
- **Non-Materializing Parser**: `cp_json` is an inspector and query engine, not a serializer or mutating DOM tree.
- **Escape Sequences**: `JsonAsView` returns raw byte slices referencing the input buffer. Escape sequences such as `\n` or `\uXXXX` remain unescaped. If escaped characters require unescaping, copy them into an `Arena` or `String`.
- **Large Container Scan Latency**: Because `cp_json` does not construct an auxiliary index, `JsonAt` on an array of 100,000 items scans sequentially from element 0. For repeated queries over huge arrays, parse elements into a `Vec(T)`.
