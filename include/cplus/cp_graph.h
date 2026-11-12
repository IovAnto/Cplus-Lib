/**
 * @file cp_graph.h
 * @brief Zero-latency Graph data structure (Adjacency List on Arena) with BFS, DFS,
 *        and Min-Heap Dijkstra Shortest Path algorithm.
 */

#ifndef CPLUS_GRAPH_H
#define CPLUS_GRAPH_H

#include "cp_core.h"
#include "cp_mem.h"
#include "cp_ds.h"

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

#endif /* CPLUS_GRAPH_H */
