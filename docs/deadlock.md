# OScope: Deadlock Detection & Graph Analytics Engine

Deadlocks represent one of the most insidious operating system failure modes: two or more threads or processes become permanently frozen waiting for resources held by one another.

OScope implements a dedicated **Wait-For Graph (WFG) Engine** that actively constructs thread/process dependency graphs, executes cycle detection algorithms, selects optimal victims, and verifies cycle resolution.

---

## 1. Graph Representation: Wait-For Graph (WFG)

In an operating system Wait-For Graph:
- **Vertices ($V$)**: Active threads or processes.
- **Directed Edges ($E$)**: An edge $T_i \longrightarrow T_j$ indicates that thread $T_i$ is blocked waiting for a mutex or resource currently held by thread $T_j$.

```
Thread 1 (Holds Lock A) ──(Waits for Lock B)──> Thread 2 (Holds Lock B)
      ^                                                |
      |                                                |
      +────────────────(Waits for Lock A)──────────────+
```

A deadlock exists if and only if there is a **directed cycle** in the Wait-For Graph.

---

## 2. Cycle Detection: Depth-First Search with Recursion Stack

OScope's C++ cycle detector implements a cycle-finding algorithm using 3-color Depth First Search:
- `WHITE` (Unvisited)
- `GRAY` (Currently in the recursion call stack)
- `BLACK` (Fully visited, all descendants explored)

### Complexity
- **Time Complexity**: $O(V + E)$
- **Space Complexity**: $O(V)$ auxiliary recursion stack

```cpp
bool CycleDetector::dfs(int u, std::vector<int>& path, std::vector<std::vector<int>>& allCycles) {
    color[u] = GRAY;
    path.push_back(u);

    for (int v : adj[u]) {
        if (color[v] == GRAY) {
            // Cycle found! Backtrack to v to isolate cycle nodes
            std::vector<int> cycle;
            auto it = std::find(path.begin(), path.end(), v);
            cycle.assign(it, path.end());
            allCycles.push_back(cycle);
        } else if (color[v] == WHITE) {
            dfs(v, path, allCycles);
        }
    }

    path.pop_back();
    color[u] = BLACK;
}
```

---

## 3. Victim Selection Strategies

When a cycle $C = \{T_1, T_2, \dots, T_k\}$ is detected, aborting all participating threads causes catastrophic unnecessary data loss. OScope provides 3 configurable victim selection strategies:

### 1. `LEAST_COST` (Default & Recommended)
Minimizes lost compute work by calculating an operating systems cost metric:
$$\text{Cost}(T) = \alpha \cdot \text{CPU\_Time}(T) + \beta \cdot \text{RSS\_MB}(T) + \gamma \cdot \text{Held\_Locks}(T)$$
The node with the lowest cost score is selected as the victim:
$$\text{Victim} = \arg\min_{T \in C} \text{Cost}(T)$$

### 2. `LOWEST_PRIORITY`
Selects the process with the lowest scheduling priority (or highest nice value):
$$\text{Victim} = \arg\min_{T \in C} \text{Priority}(T)$$

### 3. `HIGHEST_RESOURCE`
Selects the process holding the maximum number of locked resources or memory footprint, immediately freeing the largest volume of contention:
$$\text{Victim} = \arg\max_{T \in C} \text{Held\_Locks}(T)$$

---

## 4. Interactive Frontend Visualization

The OScope React dashboard includes a dedicated `/deadlocks` page featuring:
- Interactive node-link canvas rendering the Wait-For Graph.
- Red highlighting for edges forming the active deadlocked cycle.
- Detailed panel displaying detected cycle members, computed cost scores, identified victim, and real-time resolution logs.
