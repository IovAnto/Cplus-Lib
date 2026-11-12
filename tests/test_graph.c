/**
 * @file test_graph.c
 * @brief Unit tests for cp_graph: Adjacency list, BFS, DFS, and Dijkstra shortest path.
 */

#include "cplus/cplus.h"
#include <assert.h>
#include <math.h>

typedef struct {
    usize visited[16];
    usize count;
} visit_log_t;

static void test_visitor(usize node, void *user_data) {
    visit_log_t *log = (visit_log_t *)user_data;
    if (log->count < 16) {
        log->visited[log->count++] = node;
    }
}

static void test_graph_creation_and_traversal(void) {
    printf("[TEST] Graph Creation, BFS & DFS (PascalCase & Dot-Notation)...\n");

    cp_arena_t *a = ArenaCreate(8192);

    /* Directed graph with 5 nodes: 0 -> 1 -> 2, 0 -> 3 -> 4 */
    Graph g = GraphNew(a, 5, true);
    assert(GraphNodeCount(&g) == 5);

    GraphAddEdge(&g, 0, 1, 1.0);
    GraphAddEdge(&g, 1, 2, 1.0);
    GraphAddEdge(&g, 0, 3, 1.0);
    GraphAddEdge(&g, 3, 4, 1.0);
    assert(GraphEdgeCount(&g) == 4);

    /* BFS */
    visit_log_t bfs_log = { .count = 0 };
    GraphBfs(&g, 0, test_visitor, &bfs_log);
    assert(bfs_log.count == 5);
    assert(bfs_log.visited[0] == 0);

    /* DFS */
    visit_log_t dfs_log = { .count = 0 };
    GraphDfs(&g, 0, test_visitor, &dfs_log);
    assert(dfs_log.count == 5);
    assert(dfs_log.visited[0] == 0);

    /* Dot-notation API */
    Graph g_dot = graph.create(a, 3, false);
    graph.addEdge(&g_dot, 0, 1, 2.5);
    graph.addEdge(&g_dot, 1, 2, 3.5);
    assert(graph.nodeCount(&g_dot) == 3);
    assert(graph.edgeCount(&g_dot) == 2);

    visit_log_t dot_log = { .count = 0 };
    graph.bfs(&g_dot, 0, test_visitor, &dot_log);
    assert(dot_log.count == 3);

    ArenaDestroy(a);
    printf("  -> Graph Creation, BFS & DFS tests PASSED.\n");
}

static void test_graph_dijkstra(void) {
    printf("[TEST] Min-Heap Dijkstra Shortest Path...\n");

    cp_arena_t *a = ArenaCreate(16384);

    /* Undirected weighted graph:
     * (0) --4-- (1) --5-- (3) --2-- (4) --3-- (5)
     *   \        |        /          |
     *    2       1       8           10
     *     \      |      /            |
     *      ---- (2) ---              |
     */
    Graph g = GraphNew(a, 6, false);
    GraphAddEdge(&g, 0, 1, 4.0);
    GraphAddEdge(&g, 0, 2, 2.0);
    GraphAddEdge(&g, 1, 2, 1.0);
    GraphAddEdge(&g, 1, 3, 5.0);
    GraphAddEdge(&g, 2, 3, 8.0);
    GraphAddEdge(&g, 2, 4, 10.0);
    GraphAddEdge(&g, 3, 4, 2.0);
    GraphAddEdge(&g, 4, 5, 3.0);
    GraphAddEdge(&g, 3, 5, 7.0);

    f64 dist = 0.0;
    Vec(usize) path = NULL;

    /* Compute shortest path 0 -> 5 */
    b32 found = GraphDijkstra(&g, 0, 5, &dist, &path);
    assert(found == true);
    CP_UNUSED(found);
    /* 0 -> 2 (2) -> 1 (1) -> 3 (5) -> 4 (2) -> 5 (3) = 13.0 */
    assert(fabs(dist - 13.0) < 1e-6);
    assert(VecLen(path) == 6);
    assert(path[0] == 0);
    assert(path[1] == 2);
    assert(path[2] == 1);
    assert(path[3] == 3);
    assert(path[4] == 4);
    assert(path[5] == 5);

    VecFree(path);

    /* Dot notation Dijkstra test */
    f64 dist_dot = 0.0;
    Vec(usize) path_dot = NULL;
    b32 found_dot = graph.dijkstra(&g, 0, 4, &dist_dot, &path_dot);
    assert(found_dot == true);
    CP_UNUSED(found_dot);
    /* 0 -> 2 -> 1 -> 3 -> 4 = 10.0 */
    assert(fabs(dist_dot - 10.0) < 1e-6);
    assert(VecLen(path_dot) == 5);
    VecFree(path_dot);

    /* Test disconnected node */
    Graph g_disc = GraphNew(a, 3, true);
    GraphAddEdge(&g_disc, 0, 1, 5.0);
    /* Node 2 has no incoming edges */
    f64 no_dist = 0.0;
    Vec(usize) no_path = NULL;
    b32 no_found = GraphDijkstra(&g_disc, 0, 2, &no_dist, &no_path);
    assert(no_found == false);
    assert(no_path == NULL);
    CP_UNUSED(no_found);
    CP_UNUSED(no_dist);

    ArenaDestroy(a);
    printf("  -> Min-Heap Dijkstra tests PASSED.\n");
}

int main(void) {
    printf("\n=== RUNNING CPLUS GRAPH & PATHFINDING TESTS ===\n");
    test_graph_creation_and_traversal();
    test_graph_dijkstra();
    printf("=== ALL UNIT TESTS PASSED SUCCESSFULLY! ===\n\n");
    return 0;
}
