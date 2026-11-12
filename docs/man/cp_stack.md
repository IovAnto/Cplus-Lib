# Module Specification: cp_stack (L1 Cache LIFO Stack)

## 1. TL;DR
The `cp_stack` module implements a cacheline-aligned LIFO stack optimized for sub-nanosecond push and pop operations, featuring top-relative indexed peeking (`StackGet`).

```c
#include "cplus/cplus.h"

Arena *arena = ArenaCreate(64 * 1024);
Stack *s = StackCreate(16, arena);

StackPush(s, (void *)10);
StackPush(s, (void *)20);

// Index 0 is the top (20), index 1 is next down (10)
assert((intptr_t)StackGet(s, 0) == 20);
assert((intptr_t)StackGet(s, 1) == 10);

int top = (int)(intptr_t)StackPop(s);
ArenaDestroy(arena);
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Struct Layout: `Stack` (`cp_stack_t`)
```c
typedef struct cp_stack {
    void **data;
    usize top;
    usize capacity;
    cp_arena_t *arena;
} Stack;
```
- **Top-Relative Indexing**: `StackGet(s, index_from_top)` evaluates to `s->data[s->top - 1 - index_from_top]`. This eliminates tedious manual arithmetic when inspecting operands in virtual machines, AST evaluators, and recursive descent parsers.
- **Dual Memory Backing**: When `arena` is provided, the buffer grows linearly inside the arena. When `arena` is `NULL`, standard `malloc`/`realloc`/`free` is used.
- **Fast-Path Inlining**: `StackPush` and `StackPop` are implemented as inlined macros checking bounds before executing single-pointer loads/stores.

---

## 3. Operational Guarantees & Contracts

| Metric | Guarantee | Notes |
|---|---|---|
| **Push Latency** | $O(1)$ amortized | Doubles capacity when `top == capacity` |
| **Pop Latency** | $O(1)$ | Single decrement and read |
| **Peek / Indexed Get** | $O(1)$ | Direct array access relative to top |
| **Heap Allocations** | 0 when Arena-backed | 1 array buffer when heap-backed |
| **Thread-Safety** | Thread-confined | Requires `ScopedLock(&mutex)` if shared |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Create Stack | `StackCreate(cap, arena)` | `stack.create(cap, arena)` | Instantiate stack with initial capacity |
| Push Item | `StackPush(s, item)` | `stack.push(s, item)` | Push void* item onto top |
| Pop Item | `StackPop(s)` | `stack.pop(s)` | Remove and return top void* (NULL if empty) |
| Peek Top | `StackPeek(s)` | `stack.peek(s)` | Read top item without popping |
| Indexed Get | `StackGet(s, idx_from_top)` | `stack.get(s, idx_from_top)` | Read item relative to top (0 = top) |
| Stack Length | `StackLen(s)` | `stack.len(s)` | Current number of items on stack |
| Stack Capacity | `StackCap(s)` | `stack.cap(s)` | Current allocated capacity |
| Is Empty | `StackIsEmpty(s)` | `stack.isEmpty(s)` | Returns true if stack length is 0 |
| Clear Stack | `StackClear(s)` | `stack.clear(s)` | Reset top index to 0 without freeing memory |
| Destroy Stack | `StackDestroy(s)` | `stack.destroy(s)` | Deallocate stack and buffer |

---

## 5. Use Cases
1. **Reverse Polish Notation (RPN) Calculators**: Push numbers as operands; pop and evaluate on operators.
2. **Backtracking and Depth-First Traversal**: Track visited paths without recursion stack overflow.
3. **Bytecode Interpreters**: Fast operand stack for execution engines.

---

## 6. Complete C11 Usage Examples

### Example: Expression Evaluation
```c
#include "cplus/cplus.h"

int evaluate_rpn(const char *expr) {
    TempArena scratch = ScratchBegin();
    Stack *s = StackCreate(16, scratch.arena);
    
    usize count = 0;
    StrView *tokens = StrSplitArena(StrFromC(expr), ' ', scratch.arena, &count);
    
    for (usize i = 0; i < count; i++) {
        StrView t = tokens[i];
        if (StrEqualsC(t, "+")) {
            int b = (int)(intptr_t)StackPop(s);
            int a = (int)(intptr_t)StackPop(s);
            StackPush(s, (void *)(intptr_t)(a + b));
        } else if (StrEqualsC(t, "*")) {
            int b = (int)(intptr_t)StackPop(s);
            int a = (int)(intptr_t)StackPop(s);
            StackPush(s, (void *)(intptr_t)(a * b));
        } else {
            StackPush(s, (void *)(intptr_t)atoi(t.data));
        }
    }
    
    int result = (int)(intptr_t)StackPop(s);
    ScratchEnd(scratch);
    return result;
}
```

---

## 7. Limitations & Operational Constraints
- **Void Pointer Casts**: Stores items as `void*`. When storing primitive integers, cast via `(void*)(intptr_t)val` to maintain portability across 32-bit and 64-bit architectures.
- **Index Bounds in `StackGet`**: Passing `index_from_top >= StackLen(s)` safely returns `NULL`.
