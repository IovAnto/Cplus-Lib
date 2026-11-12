/**
 * @file cp_graph.h (Standalone Single-Header Module)
 * @brief Cplus standalone module for cp_graph.
 *
 * In ONE file define:
 *   #define CPLUS_CP_GRAPH_IMPLEMENTATION
 *   #include "cp_graph.h"
 */
#ifndef CPLUS_CP_GRAPH_H
#define CPLUS_CP_GRAPH_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <alloca.h>
#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Core types dependency */
#ifndef CPLUS_CORE_H
#define CPLUS_CORE_H
/**
 * @file cp_core.h
 * @brief Foundational types, platform macros, RAII defer, and polymorphic printing.
 */


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <alloca.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Exact-width & ergonomic type aliases
 * ------------------------------------------------------------------------- */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef float     f32;
typedef double    f64;

typedef uint8_t   b8;
typedef uint32_t  b32;

/* -------------------------------------------------------------------------
 * Compiler & Optimization Attributes
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) || defined(__clang__)
    #define CP_INLINE static inline __attribute__((always_inline))
    #define CP_NOINLINE __attribute__((noinline))
    #define CP_LIKELY(x) __builtin_expect(!!(x), 1)
    #define CP_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define CP_ALIGN(n) __attribute__((aligned(n)))
    #define CP_UNUSED(x) ((void)(x))
#else
    #define CP_INLINE static inline
    #define CP_NOINLINE
    #define CP_LIKELY(x) (x)
    #define CP_UNLIKELY(x) (x)
    #define CP_ALIGN(n)
    #define CP_UNUSED(x) ((void)(x))
#endif

#define CP_CACHELINE_SIZE 64
#define CP_CACHELINE CP_ALIGN(CP_CACHELINE_SIZE)

/* Default SIMD alignment */
#define CP_DEFAULT_ALIGNMENT 16
#define CP_SIMD_ALIGNMENT    32

/* Helper macro token pasting */
#define CP_CONCAT_INTERNAL_(a, b) a ## b
#define CP_CONCAT(a, b) CP_CONCAT_INTERNAL_(a, b)

/* -------------------------------------------------------------------------
 * RAII Defer mechanism
 * ------------------------------------------------------------------------- */
typedef struct {
    void (*fn)(void *);
    void *arg;
} cp_defer_t;

CP_INLINE void _cp_defer_cleanup(cp_defer_t *d) {
    if (d->fn) {
        d->fn(d->arg);
    }
}

typedef void (*cp_action_fn)(void);
CP_INLINE void _cp_defer_action_cleanup(cp_action_fn *fn) {
    if (*fn) {
        (*fn)();
    }
}

/**
 * @brief Defer execution of a function with 1 argument until the enclosing scope exits.
 * @param func_ Pointer to function taking 1 pointer argument: void func(void *arg)
 * @param arg_ Pointer passed to func when exiting scope.
 */
#define Defer(func_, arg_) \
    __attribute__((cleanup(_cp_defer_cleanup))) cp_defer_t CP_CONCAT(_cp_defer_, __LINE__) = { \
        .fn = (void (*)(void *))(func_), \
        .arg = (void *)(arg_) \
    }

/**
 * @brief Defer execution of a parameterless function until the enclosing scope exits.
 */
#define DeferAction(func_) \
    __attribute__((cleanup(_cp_defer_action_cleanup))) cp_action_fn CP_CONCAT(_cp_defact_, __LINE__) = (func_)

/* -------------------------------------------------------------------------
 * Anonymous Lambda Functions (Zero Runtime Overhead)
 * Supported natively on GCC via GNU C extension ({ ret_type fn args body fn; }).
 * On Clang / MSVC without nested function syntax, use static functions or
 * zero-overhead iteration macros (VecEach, VecMap).
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) && !defined(__clang__) && !defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__)
#define CP_HAS_LAMBDA 1
/**
 * @brief Define an anonymous inlined lambda function with zero runtime overhead (GCC).
 * @example AlgoSort(arr, len, sizeof(int), Lambda(int, (const void *a, const void *b), { return *(int*)a - *(int*)b; }));
 */
#define Lambda(ret_type, args, body) __extension__ ({ ret_type _cp_lambda args body _cp_lambda; })
#else
#define CP_HAS_LAMBDA 0
#endif

/* -------------------------------------------------------------------------
 * Polymorphic Type-Safe Printing (C11 _Generic)
 * ------------------------------------------------------------------------- */
CP_INLINE void _cp_print_bool(bool v) { fputs(v ? "true" : "false", stdout); }
CP_INLINE void _cp_print_char(char v) { putchar(v); }
CP_INLINE void _cp_print_i64(long long v) { printf("%lld", v); }
CP_INLINE void _cp_print_u64(unsigned long long v) { printf("%llu", v); }
CP_INLINE void _cp_print_f64(double v) { printf("%.6g", v); }
CP_INLINE void _cp_print_str(const char *v) { fputs(v ? v : "(null)", stdout); }
CP_INLINE void _cp_print_ptr(const void *v) { printf("%p", v); }

#define Print(val) _Generic((val), \
    _Bool: _cp_print_bool, \
    char: _cp_print_char, \
    signed char: _cp_print_i64, \
    short: _cp_print_i64, \
    int: _cp_print_i64, \
    long: _cp_print_i64, \
    long long: _cp_print_i64, \
    unsigned char: _cp_print_u64, \
    unsigned short: _cp_print_u64, \
    unsigned int: _cp_print_u64, \
    unsigned long: _cp_print_u64, \
    unsigned long long: _cp_print_u64, \
    float: _cp_print_f64, \
    double: _cp_print_f64, \
    char*: _cp_print_str, \
    const char*: _cp_print_str, \
    default: _cp_print_ptr \
)(val)

#define Println(val) do { Print(val); putchar('\n'); } while(0)
#define PrintF(fmt, ...) printf(fmt, __VA_ARGS__)

/* -------------------------------------------------------------------------
 * Panic & Assertions
 * ------------------------------------------------------------------------- */
#define Panic(msg) do { \
    fprintf(stderr, "\n[CPLUS PANIC] %s:%d: %s\n", __FILE__, __LINE__, msg); \
    abort(); \
} while(0)

#define AssertMsg(cond, msg) do { \
    if (CP_UNLIKELY(!(cond))) { \
        fprintf(stderr, "\n[CPLUS ASSERTION FAILED] %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, msg); \
        abort(); \
    } \
} while(0)

/* -------------------------------------------------------------------------
 * Bit Manipulation Intrinsics
 * ------------------------------------------------------------------------- */
CP_INLINE u32 cp_bit_popcount32(u32 x) { return (u32)__builtin_popcount(x); }
CP_INLINE u32 cp_bit_popcount64(u64 x) { return (u32)__builtin_popcountll(x); }
CP_INLINE u32 cp_bit_clz32(u32 x) { return x == 0 ? 32 : (u32)__builtin_clz(x); }
CP_INLINE u32 cp_bit_clz64(u64 x) { return x == 0 ? 64 : (u32)__builtin_clzll(x); }
CP_INLINE u32 cp_bit_ctz32(u32 x) { return x == 0 ? 32 : (u32)__builtin_ctz(x); }
CP_INLINE u32 cp_bit_ctz64(u64 x) { return x == 0 ? 64 : (u32)__builtin_ctzll(x); }
CP_INLINE u16 cp_bit_bswap16(u16 x) { return __builtin_bswap16(x); }
CP_INLINE u32 cp_bit_bswap32(u32 x) { return __builtin_bswap32(x); }
CP_INLINE u64 cp_bit_bswap64(u64 x) { return __builtin_bswap64(x); }
CP_INLINE u32 cp_bit_rotl32(u32 x, u32 s) { return (x << (s & 31)) | (x >> ((32 - (s & 31)) & 31)); }
CP_INLINE u32 cp_bit_rotr32(u32 x, u32 s) { return (x >> (s & 31)) | (x << ((32 - (s & 31)) & 31)); }
CP_INLINE u64 cp_bit_rotl64(u64 x, u32 s) { return (x << (s & 63)) | (x >> ((64 - (s & 63)) & 63)); }
CP_INLINE u64 cp_bit_rotr64(u64 x, u32 s) { return (x >> (s & 63)) | (x << ((64 - (s & 63)) & 63)); }
CP_INLINE bool cp_bit_is_pow2(u64 x) { return (x != 0) && ((x & (x - 1)) == 0); }
CP_INLINE u64 cp_bit_next_pow2(u64 x) {
    if (x <= 1) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

#define BitPopcount32(x) cp_bit_popcount32(x)
#define BitPopcount64(x) cp_bit_popcount64(x)
#define BitClz32(x)      cp_bit_clz32(x)
#define BitClz64(x)      cp_bit_clz64(x)
#define BitCtz32(x)      cp_bit_ctz32(x)
#define BitCtz64(x)      cp_bit_ctz64(x)
#define BitBswap16(x)    cp_bit_bswap16(x)
#define BitBswap32(x)    cp_bit_bswap32(x)
#define BitBswap64(x)    cp_bit_bswap64(x)
#define BitRotl32(x, s)  cp_bit_rotl32(x, s)
#define BitRotr32(x, s)  cp_bit_rotr32(x, s)
#define BitRotl64(x, s)  cp_bit_rotl64(x, s)
#define BitRotr64(x, s)  cp_bit_rotr64(x, s)
#define BitIsPow2(x)     cp_bit_is_pow2(x)
#define BitNextPow2(x)   cp_bit_next_pow2(x)

typedef struct {
    u32 (*const popcount32)(u32 x);
    u32 (*const popcount64)(u64 x);
    u32 (*const clz32)(u32 x);
    u32 (*const clz64)(u64 x);
    u32 (*const ctz32)(u32 x);
    u32 (*const ctz64)(u64 x);
    u16 (*const bswap16)(u16 x);
    u32 (*const bswap32)(u32 x);
    u64 (*const bswap64)(u64 x);
    u32 (*const rotl32)(u32 x, u32 s);
    u32 (*const rotr32)(u32 x, u32 s);
    u64 (*const rotl64)(u64 x, u32 s);
    u64 (*const rotr64)(u64 x, u32 s);
    bool (*const isPow2)(u64 x);
    u64 (*const nextPow2)(u64 x);
} cp_bit_namespace_t;

static const cp_bit_namespace_t bit = {
    .popcount32 = cp_bit_popcount32,
    .popcount64 = cp_bit_popcount64,
    .clz32 = cp_bit_clz32,
    .clz64 = cp_bit_clz64,
    .ctz32 = cp_bit_ctz32,
    .ctz64 = cp_bit_ctz64,
    .bswap16 = cp_bit_bswap16,
    .bswap32 = cp_bit_bswap32,
    .bswap64 = cp_bit_bswap64,
    .rotl32 = cp_bit_rotl32,
    .rotr32 = cp_bit_rotr32,
    .rotl64 = cp_bit_rotl64,
    .rotr64 = cp_bit_rotr64,
    .isPow2 = cp_bit_is_pow2,
    .nextPow2 = cp_bit_next_pow2,
};

/* -------------------------------------------------------------------------
 * Colored Diagnostic Logging
 * ------------------------------------------------------------------------- */
typedef enum {
    CP_LOG_LEVEL_DEBUG = 0,
    CP_LOG_LEVEL_INFO  = 1,
    CP_LOG_LEVEL_WARN  = 2,
    CP_LOG_LEVEL_ERROR = 3,
    CP_LOG_LEVEL_OFF   = 4,
} cp_log_level_t;

static inline cp_log_level_t *_cp_log_level_ptr(void) {
    static cp_log_level_t lvl = CP_LOG_LEVEL_INFO;
    return &lvl;
}

CP_INLINE void cp_log_set_level(cp_log_level_t lvl) {
    *_cp_log_level_ptr() = lvl;
}

CP_INLINE cp_log_level_t cp_log_get_level(void) {
    return *_cp_log_level_ptr();
}

#define LogSetLevel(lvl) cp_log_set_level(lvl)

#define LogDebug(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_DEBUG) { \
        fprintf(stderr, "\033[90m[DEBUG]\033[0m " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogInfo(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_INFO) { \
        fprintf(stdout, "\033[32m[INFO]\033[0m  " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogWarn(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_WARN) { \
        fprintf(stderr, "\033[33m[WARN]\033[0m  " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define LogError(fmt, ...) do { \
    if (cp_log_get_level() <= CP_LOG_LEVEL_ERROR) { \
        fprintf(stderr, "\033[1;31m[ERROR]\033[0m " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif
#endif /* CPLUS_CORE_H */

/**
 * @file cp_graph.h
 * @brief Zero-latency Graph data structure (Adjacency List on Arena) with BFS, DFS,
 *        and Min-Heap Dijkstra Shortest Path algorithm.
 */



#ifdef __cplusplus
extern "C" {
#endif

typedef struct cp_graph_edge {
    usize to;
    f64 weight;
    struct cp_graph_edge *next;
} cp_graph_edge_t;

typedef cp_graph_edge_t GraphEdge;

typedef struct cp_graph {
    usize node_count;
    usize edge_count;
    cp_graph_edge_t **adj;
    b32 is_directed;
    cp_arena_t *arena;
} cp_graph_t;

typedef cp_graph_t Graph;

typedef void (*cp_graph_visitor_fn)(usize node, void *user_data);

/* -------------------------------------------------------------------------
 * Graph Construction & Query Operations
 * ------------------------------------------------------------------------- */
cp_graph_t cp_graph_new(cp_arena_t *arena, usize node_count, b32 is_directed);
void cp_graph_add_edge(cp_graph_t *g, usize from, usize to, f64 weight);
usize cp_graph_node_count(const cp_graph_t *g);
usize cp_graph_edge_count(const cp_graph_t *g);

/* -------------------------------------------------------------------------
 * Graph Traversal & Shortest Path Algorithms
 * ------------------------------------------------------------------------- */
void cp_graph_bfs(const cp_graph_t *g, usize start_node, cp_graph_visitor_fn visitor, void *user_data);
void cp_graph_dfs(const cp_graph_t *g, usize start_node, cp_graph_visitor_fn visitor, void *user_data);

/**
 * @brief Computes shortest path using Min-Heap optimized Dijkstra algorithm.
 * @param g Graph instance
 * @param start Source node index
 * @param target Destination node index
 * @param out_dist Optional pointer to store computed shortest distance
 * @param out_path Optional pointer to dynamic Vec(usize) populated with node sequence
 * @return true if path exists from start to target, false otherwise
 */
b32 cp_graph_dijkstra(const cp_graph_t *g, usize start, usize target, f64 *out_dist, Vec(usize) *out_path);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define GraphNew(arena, nodes, directed) cp_graph_new(arena, nodes, directed)
#define GraphAddEdge(g, u, v, w)         cp_graph_add_edge(g, u, v, w)
#define GraphNodeCount(g)                cp_graph_node_count(g)
#define GraphEdgeCount(g)                cp_graph_edge_count(g)
#define GraphBfs(g, start, fn, data)     cp_graph_bfs(g, start, fn, data)
#define GraphDfs(g, start, fn, data)     cp_graph_dfs(g, start, fn, data)
#define GraphDijkstra(g, s, t, dist, p)  cp_graph_dijkstra(g, s, t, dist, p)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    cp_graph_t (*const create)(cp_arena_t *arena, usize node_count, b32 is_directed);
    void (*const addEdge)(cp_graph_t *g, usize from, usize to, f64 weight);
    usize (*const nodeCount)(const cp_graph_t *g);
    usize (*const edgeCount)(const cp_graph_t *g);
    void (*const bfs)(const cp_graph_t *g, usize start, cp_graph_visitor_fn visitor, void *user_data);
    void (*const dfs)(const cp_graph_t *g, usize start, cp_graph_visitor_fn visitor, void *user_data);
    b32 (*const dijkstra)(const cp_graph_t *g, usize start, usize target, f64 *out_dist, Vec(usize) *out_path);
} cp_graph_namespace_t;

static const cp_graph_namespace_t graph = {
    .create = cp_graph_new,
    .addEdge = cp_graph_add_edge,
    .nodeCount = cp_graph_node_count,
    .edgeCount = cp_graph_edge_count,
    .bfs = cp_graph_bfs,
    .dfs = cp_graph_dfs,
    .dijkstra = cp_graph_dijkstra,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_CP_GRAPH_H */

#ifdef CPLUS_CP_GRAPH_IMPLEMENTATION
/**
 * @file cp_graph.c
 * @brief High-performance Graph implementation with Adjacency List and Min-Heap Dijkstra.
 */

#include <stdlib.h>
#include <string.h>
#include <float.h>

/* -------------------------------------------------------------------------
 * Graph Creation & Edge Operations
 * ------------------------------------------------------------------------- */
cp_graph_t cp_graph_new(cp_arena_t *arena, usize node_count, b32 is_directed) {
    if (!arena) arena = Scratch();

    cp_graph_t g;
    g.node_count = node_count;
    g.edge_count = 0;
    g.is_directed = is_directed;
    g.arena = arena;
    g.adj = (cp_graph_edge_t **)ArenaAllocZero(arena, node_count * sizeof(cp_graph_edge_t *));
    return g;
}

static cp_graph_edge_t *_cp_alloc_edge(cp_arena_t *arena, usize to, f64 weight, cp_graph_edge_t *next) {
    cp_graph_edge_t *e = (cp_graph_edge_t *)ArenaAlloc(arena, sizeof(cp_graph_edge_t));
    e->to = to;
    e->weight = weight;
    e->next = next;
    return e;
}

void cp_graph_add_edge(cp_graph_t *g, usize from, usize to, f64 weight) {
    if (!g || from >= g->node_count || to >= g->node_count) return;

    g->adj[from] = _cp_alloc_edge(g->arena, to, weight, g->adj[from]);
    if (!g->is_directed && from != to) {
        g->adj[to] = _cp_alloc_edge(g->arena, from, weight, g->adj[to]);
    }
    g->edge_count++;
}

usize cp_graph_node_count(const cp_graph_t *g) {
    return g ? g->node_count : 0;
}

usize cp_graph_edge_count(const cp_graph_t *g) {
    return g ? g->edge_count : 0;
}

/* -------------------------------------------------------------------------
 * Graph Traversals (BFS & DFS)
 * ------------------------------------------------------------------------- */
void cp_graph_bfs(const cp_graph_t *g, usize start_node, cp_graph_visitor_fn visitor, void *user_data) {
    if (!g || start_node >= g->node_count || !visitor) return;

    cp_temp_arena_t scratch = ScratchBegin();
    b8 *visited = (b8 *)ArenaAllocZero(scratch.arena, g->node_count * sizeof(b8));
    usize *queue = (usize *)ArenaAlloc(scratch.arena, g->node_count * sizeof(usize));

    usize q_head = 0;
    usize q_tail = 0;

    visited[start_node] = 1;
    queue[q_tail++] = start_node;

    while (q_head < q_tail) {
        usize u = queue[q_head++];
        visitor(u, user_data);

        for (cp_graph_edge_t *e = g->adj[u]; e != NULL; e = e->next) {
            if (!visited[e->to]) {
                visited[e->to] = 1;
                queue[q_tail++] = e->to;
            }
        }
    }

    ScratchEnd(scratch);
}

void cp_graph_dfs(const cp_graph_t *g, usize start_node, cp_graph_visitor_fn visitor, void *user_data) {
    if (!g || start_node >= g->node_count || !visitor) return;

    cp_temp_arena_t scratch = ScratchBegin();
    b8 *visited = (b8 *)ArenaAllocZero(scratch.arena, g->node_count * sizeof(b8));
    usize *stack = (usize *)ArenaAlloc(scratch.arena, g->node_count * sizeof(usize));

    usize stack_top = 0;
    stack[stack_top++] = start_node;

    while (stack_top > 0) {
        usize u = stack[--stack_top];
        if (visited[u]) continue;

        visited[u] = 1;
        visitor(u, user_data);

        /* Push neighbors in reverse to maintain left-to-right visit order */
        for (cp_graph_edge_t *e = g->adj[u]; e != NULL; e = e->next) {
            if (!visited[e->to]) {
                stack[stack_top++] = e->to;
            }
        }
    }

    ScratchEnd(scratch);
}

/* -------------------------------------------------------------------------
 * Min-Heap Priority Queue for Dijkstra
 * ------------------------------------------------------------------------- */
typedef struct {
    usize node;
    f64 dist;
} cp_heap_entry_t;

typedef struct {
    cp_heap_entry_t *data;
    usize size;
    usize capacity;
} cp_min_heap_t;

static cp_min_heap_t _cp_heap_create(cp_arena_t *arena, usize capacity) {
    cp_min_heap_t h;
    h.data = (cp_heap_entry_t *)ArenaAlloc(arena, capacity * sizeof(cp_heap_entry_t));
    h.size = 0;
    h.capacity = capacity;
    return h;
}

static void _cp_heap_push(cp_min_heap_t *h, usize node, f64 dist) {
    if (h->size >= h->capacity) return;

    usize i = h->size++;
    h->data[i] = (cp_heap_entry_t){ node, dist };

    while (i > 0) {
        usize parent = (i - 1) / 2;
        if (h->data[parent].dist <= h->data[i].dist) break;

        cp_heap_entry_t temp = h->data[parent];
        h->data[parent] = h->data[i];
        h->data[i] = temp;
        i = parent;
    }
}

static b32 _cp_heap_pop(cp_min_heap_t *h, cp_heap_entry_t *out_entry) {
    if (h->size == 0) return false;

    *out_entry = h->data[0];
    h->data[0] = h->data[--h->size];

    usize i = 0;
    while (2 * i + 1 < h->size) {
        usize left = 2 * i + 1;
        usize right = 2 * i + 2;
        usize smallest = left;

        if (right < h->size && h->data[right].dist < h->data[left].dist) {
            smallest = right;
        }

        if (h->data[i].dist <= h->data[smallest].dist) break;

        cp_heap_entry_t temp = h->data[i];
        h->data[i] = h->data[smallest];
        h->data[smallest] = temp;
        i = smallest;
    }

    return true;
}

/* -------------------------------------------------------------------------
 * Dijkstra Shortest Path Algorithm Implementation
 * ------------------------------------------------------------------------- */
b32 cp_graph_dijkstra(const cp_graph_t *g, usize start, usize target, f64 *out_dist, Vec(usize) *out_path) {
    if (!g || start >= g->node_count || target >= g->node_count) return false;

    cp_temp_arena_t scratch = ScratchBegin();

    f64 *dist = (f64 *)ArenaAlloc(scratch.arena, g->node_count * sizeof(f64));
    usize *prev = (usize *)ArenaAlloc(scratch.arena, g->node_count * sizeof(usize));

    for (usize i = 0; i < g->node_count; i++) {
        dist[i] = DBL_MAX;
        prev[i] = (usize)-1;
    }

    dist[start] = 0.0;
    usize heap_cap = (g->edge_count > 0 ? g->edge_count * 2 : 16) + 16;
    cp_min_heap_t heap = _cp_heap_create(scratch.arena, heap_cap);
    _cp_heap_push(&heap, start, 0.0);

    cp_heap_entry_t current;
    while (_cp_heap_pop(&heap, &current)) {
        usize u = current.node;
        f64 d = current.dist;

        if (d > dist[u]) continue;
        if (u == target) break; /* Reached target node */

        for (cp_graph_edge_t *e = g->adj[u]; e != NULL; e = e->next) {
            usize v = e->to;
            f64 new_dist = dist[u] + e->weight;

            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                prev[v] = u;
                _cp_heap_push(&heap, v, new_dist);
            }
        }
    }

    if (dist[target] == DBL_MAX) {
        ScratchEnd(scratch);
        return false; /* No path exists */
    }

    if (out_dist) {
        *out_dist = dist[target];
    }

    if (out_path) {
        /* Reconstruct path backwards */
        Vec(usize) path = VecNew(usize);
        usize curr = target;
        while (curr != (usize)-1) {
            VecPush(path, curr);
            curr = prev[curr];
        }

        /* Reverse path */
        usize len = VecLen(path);
        for (usize i = 0; i < len / 2; i++) {
            usize tmp = path[i];
            path[i] = path[len - 1 - i];
            path[len - 1 - i] = tmp;
        }
        *out_path = path;
    }

    ScratchEnd(scratch);
    return true;
}
#endif /* CPLUS_CP_GRAPH_IMPLEMENTATION */
