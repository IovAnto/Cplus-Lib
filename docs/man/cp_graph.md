# Module Specification: cp_graph (Graph Algorithms & Priority Queues)

## 1. TL;DR
The `cp_graph` module provides adjacency-list graph representations backed entirely by linear `Arena` memory, supporting Breadth-First Search (BFS), Depth-First Search (DFS), and Dijkstra's Shortest Path algorithm with a 4-ary min-heap priority queue.

```c
#include "cplus/cplus.h"

Arena *arena = ArenaCreate(64 * 1024);
Graph g = GraphNew(arena, 4, false); // 4 nodes, undirected

GraphAddEdge(&g, 0, 1, 2.0);
GraphAddEdge(&g, 1, 2, 3.5);
GraphAddEdge(&g, 2, 3, 1.0);

f64 dist[4];
usize prev[4];
GraphDijkstra(&g, 0, dist, prev);

printf("Shortest path from 0 to 3: %.2f\n", dist[3]); // 6.50
ArenaDestroy(arena); // 0 memory leaks
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Struct Layout: `Graph` (`cp_graph_t`) and `GraphEdge` (`cp_graph_edge_t`)
```c
typedef struct cp_graph_edge {
    usize to;
    f64 weight;
    struct cp_graph_edge *next;
} GraphEdge;

typedef struct cp_graph {
    usize node_count;
    usize edge_count;
    GraphEdge **adj;
    b32 is_directed;
    cp_arena_t *arena;
} Graph;
```
- **Zero Heap Fragmentation**: The adjacency list node heads (`adj`) and all `GraphEdge` link nodes are allocated contiguously from an `Arena`. Building a 100,000-edge graph issues 0 individual `malloc()` calls.
- **4-ary Min-Heap Dijkstra**: `GraphDijkstra` utilizes a 4-ary min-heap priority queue. 4-ary heaps feature shallower tree depths than standard binary heaps, increasing L1 cacheline hit rates during heap sift-up and sift-down operations.

---

## 3. Operational Guarantees & Contracts

| Metric | Guarantee | Notes |
|---|---|---|
| **Add Edge** | $O(1)$ | Prepends edge node to adjacency list |
| **BFS / DFS** | $O(V + E)$ | Linear traversal of reachable vertices |
| **Dijkstra Shortest Path**| $O((V + E) \log V)$ | Accelerated via 4-ary min-heap |
| **Memory Allocation** | Linear Arena allocation | Reclaimed entirely via `ArenaDestroy` |
| **Thread-Safety** | Thread-confined | Safe for concurrent reads if not mutated |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| New Graph | `GraphNew(arena, nodes, directed)`| `graph.create(...)` | Initialize graph with node count |
| Add Edge | `GraphAddEdge(g, from, to, weight)`| `graph.addEdge(...)` | Insert weighted edge (undirected inserts both) |
| Node Count | `GraphNodeCount(g)` | `graph.nodeCount(g)` | Query total number of vertices |
| Edge Count | `GraphEdgeCount(g)` | `graph.edgeCount(g)` | Query total number of edges |
| BFS Traversal | `GraphBfs(g, start, visitor, data)` | `graph.bfs(...)` | Breadth-First Search |
| DFS Traversal | `GraphDfs(g, start, visitor, data)` | `graph.dfs(...)` | Depth-First Search |
| Dijkstra | `GraphDijkstra(g, start, dist, prev)`| `graph.dijkstra(...)` | Single-source shortest path |

---

## 5. Use Cases
1. **Network Topology & Routing**: Compute lowest-latency paths between network nodes using `GraphDijkstra`.
2. **Dependency Resolution**: Topological sorting and reachability checks for build systems and package managers.
3. **Game Navigation Meshes (NavMesh)**: Fast pathfinding across polygonal navigation graph waypoints.

---

## 6. Complete C11 Usage Examples

### Example: BFS Visitor
```c
#include "cplus/cplus.h"

void print_node(usize node, void *user_data) {
    printf("Visited node %zu\n", node);
}

void traverse_network(void) {
    TempArena scratch = ScratchBegin();
    Graph g = GraphNew(scratch.arena, 5, true);
    
    GraphAddEdge(&g, 0, 1, 1.0);
    GraphAddEdge(&g, 0, 2, 1.0);
    GraphAddEdge(&g, 1, 3, 1.0);
    GraphAddEdge(&g, 2, 4, 1.0);
    
    printf("--- BFS Traversal ---\n");
    GraphBfs(&g, 0, print_node, NULL);
    
    ScratchEnd(scratch);
}
```

---

## 7. Limitations & Operational Constraints
- **Fixed Node Count**: `GraphNew` fixes the maximum number of nodes at initialization. Node IDs are 0-indexed integers from `0` to `node_count - 1`. If your vertices have string names, map them to integers using `HashMap`.
- **Non-Negative Weights**: `GraphDijkstra` requires non-negative edge weights ($w \ge 0$). Graphs with negative edges require Bellman-Ford (not included).
