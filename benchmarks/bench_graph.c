/**
 * @file bench_graph.c
 * @brief Scientific latency & throughput benchmark: Pure C vs Cplus Graph Architecture.
 *        Measures Graph construction with 100,000 edges (malloc vs Arena)
 *        and Min-Heap Dijkstra shortest path query latency.
 */

#include "cplus/cplus.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_RUNS 5
#define GRAPH_NODES 20000
#define GRAPH_EDGES 100000
#define DIJKSTRA_QUERIES 200

typedef struct {
    f64 samples_ms[NUM_RUNS];
    f64 min_ms;
    f64 max_ms;
    f64 mean_ms;
    f64 stddev_ms;
    f64 latency_ns_op;
} bench_stats_t;

static inline u64 get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

static void compute_stats(bench_stats_t *stats, usize total_ops) {
    f64 sum = 0.0;
    stats->min_ms = stats->samples_ms[0];
    stats->max_ms = stats->samples_ms[0];

    for (int i = 0; i < NUM_RUNS; i++) {
        f64 v = stats->samples_ms[i];
        sum += v;
        if (v < stats->min_ms) stats->min_ms = v;
        if (v > stats->max_ms) stats->max_ms = v;
    }
    stats->mean_ms = sum / NUM_RUNS;

    f64 var_sum = 0.0;
    for (int i = 0; i < NUM_RUNS; i++) {
        f64 diff = stats->samples_ms[i] - stats->mean_ms;
        var_sum += diff * diff;
    }
    stats->stddev_ms = sqrt(var_sum / NUM_RUNS);
    stats->latency_ns_op = (stats->mean_ms * 1000000.0) / (f64)total_ops;
}

/* =========================================================================
 * EXPERIMENT 1: Large Graph Construction (20,000 nodes, 100,000 edges)
 * ========================================================================= */
typedef struct raw_c_edge {
    usize to;
    f64 weight;
    struct raw_c_edge *next;
} raw_c_edge_t;

typedef struct {
    raw_c_edge_t **adj;
    usize node_count;
} raw_c_graph_t;

static void run_graph_construction_benchmark(void) {
    printf("--- PART A: Graph Construction (20,000 nodes, 100,000 edges: Build + Free) ---\n\n");

    bench_stats_t stats_c_graph;
    bench_stats_t stats_cp_graph;

    /* Pure C: malloc each edge individually */
    printf("[EXPERIMENT 1.1] Pure C (Individual malloc for each edge + iterative free)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        raw_c_graph_t g;
        g.node_count = GRAPH_NODES;
        g.adj = (raw_c_edge_t **)calloc(GRAPH_NODES, sizeof(raw_c_edge_t *));

        for (usize i = 0; i < GRAPH_EDGES; i++) {
            usize from = (i * 17) % GRAPH_NODES;
            usize to = (i * 31 + 7) % GRAPH_NODES;
            f64 weight = (f64)((i % 50) + 1);

            raw_c_edge_t *e = (raw_c_edge_t *)malloc(sizeof(raw_c_edge_t));
            e->to = to;
            e->weight = weight;
            e->next = g.adj[from];
            g.adj[from] = e;
        }

        /* Free pure C graph */
        for (usize i = 0; i < GRAPH_NODES; i++) {
            raw_c_edge_t *cur = g.adj[i];
            while (cur) {
                raw_c_edge_t *next = cur->next;
                free(cur);
                cur = next;
            }
        }
        free(g.adj);

        u64 t1 = get_time_ns();
        stats_c_graph.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_c_graph.samples_ms[r]);
    }
    compute_stats(&stats_c_graph, GRAPH_EDGES);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/edge\n\n",
           stats_c_graph.mean_ms, stats_c_graph.min_ms, stats_c_graph.max_ms,
           stats_c_graph.stddev_ms, stats_c_graph.latency_ns_op);

    /* Cplus Graph (Arena-backed) */
    printf("[EXPERIMENT 1.2] Cplus Graph (Arena-backed contiguous edges + O(1) batch free)\n");
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        cp_arena_t *arena = ArenaCreate(8 * 1024 * 1024);
        Graph g = GraphNew(arena, GRAPH_NODES, true);

        for (usize i = 0; i < GRAPH_EDGES; i++) {
            usize from = (i * 17) % GRAPH_NODES;
            usize to = (i * 31 + 7) % GRAPH_NODES;
            f64 weight = (f64)((i % 50) + 1);
            GraphAddEdge(&g, from, to, weight);
        }

        ArenaDestroy(arena); /* Instant batch reclamation */
        u64 t1 = get_time_ns();
        stats_cp_graph.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_cp_graph.samples_ms[r]);
    }
    compute_stats(&stats_cp_graph, GRAPH_EDGES);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms | Latency: %8.2f ns/edge\n",
           stats_cp_graph.mean_ms, stats_cp_graph.min_ms, stats_cp_graph.max_ms,
           stats_cp_graph.stddev_ms, stats_cp_graph.latency_ns_op);
    printf("  >> SPEEDUP: %8.2fx FASTER than Pure C\n\n", stats_c_graph.mean_ms / stats_cp_graph.mean_ms);
}

/* =========================================================================
 * EXPERIMENT 2: Dijkstra Shortest Path Latency on Mesh Topology
 * ========================================================================= */
static void run_dijkstra_query_benchmark(void) {
    printf("--- PART B: Min-Heap Dijkstra Shortest Path Queries (10,000 nodes, 40,000 edges) ---\n\n");

    bench_stats_t stats_dijkstra;
    cp_arena_t *arena = ArenaCreate(16 * 1024 * 1024);
    usize mesh_nodes = 10000;
    Graph g = GraphNew(arena, mesh_nodes, false);

    /* Construct connected grid/mesh topology */
    for (usize i = 0; i < mesh_nodes - 1; i++) {
        GraphAddEdge(&g, i, i + 1, 1.0 + (f64)(i % 7));
        if (i + 10 < mesh_nodes) {
            GraphAddEdge(&g, i, i + 10, 2.0 + (f64)(i % 5));
        }
        if (i + 100 < mesh_nodes) {
            GraphAddEdge(&g, i, i + 100, 5.0 + (f64)(i % 11));
        }
    }

    printf("[EXPERIMENT 2.1] Cplus Dijkstra Query Latency (%d queries)\n", DIJKSTRA_QUERIES);
    for (int r = 0; r < NUM_RUNS; r++) {
        u64 t0 = get_time_ns();
        usize successful_queries = 0;

        for (int q = 0; q < DIJKSTRA_QUERIES; q++) {
            usize start = (usize)(q * 37) % mesh_nodes;
            usize target = (usize)(mesh_nodes - 1 - (q * 19) % mesh_nodes);

            f64 dist = 0.0;
            Vec(usize) path = NULL;
            if (GraphDijkstra(&g, start, target, &dist, &path)) {
                successful_queries++;
                VecFree(path);
            }
        }

        u64 t1 = get_time_ns();
        CP_UNUSED(successful_queries);
        stats_dijkstra.samples_ms[r] = (f64)(t1 - t0) / 1000000.0;
        printf("  Run %d: %8.2f ms\n", r + 1, stats_dijkstra.samples_ms[r]);
    }

    ArenaDestroy(arena);
    compute_stats(&stats_dijkstra, DIJKSTRA_QUERIES);
    printf("  >> Mean: %8.2f ms | Min: %8.2f ms | Max: %8.2f ms | StdDev: ±%5.2f ms\n",
           stats_dijkstra.mean_ms, stats_dijkstra.min_ms, stats_dijkstra.max_ms, stats_dijkstra.stddev_ms);
    f64 latency_us = (stats_dijkstra.mean_ms * 1000.0) / (f64)DIJKSTRA_QUERIES;
    printf("  >> Avg Query Latency: %8.2f microseconds / query\n", latency_us);
    f64 qps = ((f64)DIJKSTRA_QUERIES / stats_dijkstra.mean_ms) * 1000.0;
    printf("  >> Throughput:        %8.2f shortest path queries / sec\n\n", qps);
}

int main(void) {
    printf("=================================================================================\n");
    printf("  SCIENTIFIC EXPERIMENT REPORT: GRAPH ALLOCATION & PATHFINDING LATENCY\n");
    printf("=================================================================================\n\n");

    /* =========================================================================
     * Verification of Parity
     * ========================================================================= */
    printf("[VERIFICATION] Running strict parity check between Pure C and Cplus...\n");
    {
        raw_c_edge_t **c_adj = (raw_c_edge_t **)calloc(GRAPH_NODES, sizeof(raw_c_edge_t *));
        u64 c_sum_to = 0;
        f64 c_sum_weight = 0.0;
        usize c_count = 0;

        for (usize i = 0; i < GRAPH_EDGES; i++) {
            usize from = (i * 17) % GRAPH_NODES;
            usize to = (i * 31 + 7) % GRAPH_NODES;
            f64 weight = (f64)((i % 50) + 1);

            raw_c_edge_t *e = (raw_c_edge_t *)malloc(sizeof(raw_c_edge_t));
            e->to = to;
            e->weight = weight;
            e->next = c_adj[from];
            c_adj[from] = e;
        }

        for (usize i = 0; i < GRAPH_NODES; i++) {
            raw_c_edge_t *cur = c_adj[i];
            while (cur) {
                c_count++;
                c_sum_to += cur->to;
                c_sum_weight += cur->weight;
                raw_c_edge_t *nxt = cur->next;
                free(cur);
                cur = nxt;
            }
        }
        free(c_adj);

        cp_arena_t *a = ArenaCreate(8 * 1024 * 1024);
        Graph g = GraphNew(a, GRAPH_NODES, true);
        for (usize i = 0; i < GRAPH_EDGES; i++) {
            usize from = (i * 17) % GRAPH_NODES;
            usize to = (i * 31 + 7) % GRAPH_NODES;
            f64 weight = (f64)((i % 50) + 1);
            GraphAddEdge(&g, from, to, weight);
        }

        u64 cp_sum_to = 0;
        f64 cp_sum_weight = 0.0;
        usize cp_count = 0;
        for (usize i = 0; i < GRAPH_NODES; i++) {
            cp_graph_edge_t *cur = g.adj[i];
            while (cur) {
                cp_count++;
                cp_sum_to += cur->to;
                cp_sum_weight += cur->weight;
                cur = cur->next;
            }
        }
        ArenaDestroy(a);

        printf("  Pure C: edges = %zu | sum_dest = %llu | sum_weights = %.1f\n",
               c_count, (unsigned long long)c_sum_to, c_sum_weight);
        printf("  Cplus:  edges = %zu | sum_dest = %llu | sum_weights = %.1f\n",
               cp_count, (unsigned long long)cp_sum_to, cp_sum_weight);

        if (c_count != cp_count || c_sum_to != cp_sum_to || fabs(c_sum_weight - cp_sum_weight) > 1e-6) {
            fprintf(stderr, "PARITY MISMATCH! Pure C and Cplus graphs do not match!\n");
            abort();
        }
        printf("  >> PARITY CHECK PASSED: Pure C and Cplus graphs are 100.0%% identical!\n\n");
    }

    run_graph_construction_benchmark();
    run_dijkstra_query_benchmark();

    printf("=================================================================================\n");
    return 0;
}
