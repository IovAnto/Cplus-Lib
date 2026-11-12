/**
 * @file cp_graph.c
 * @brief High-performance Graph implementation with Adjacency List and Min-Heap Dijkstra.
 */

#include "cplus/cp_graph.h"
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
