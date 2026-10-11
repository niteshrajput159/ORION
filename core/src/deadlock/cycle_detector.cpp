#include "deadlock/cycle_detector.hpp"
#include <algorithm>

namespace oscope {

bool CycleDetector::dfs(int current,
                        const std::map<int, std::vector<int>>& adj,
                        std::map<int, NodeColor>& colors,
                        std::map<int, int>& parent,
                        std::vector<int>& cycle) {
    colors[current] = NodeColor::GRAY;

    auto it = adj.find(current);
    if (it != adj.end()) {
        for (int neighbor : it->second) {
            if (colors[neighbor] == NodeColor::GRAY) {
                // Cycle detected! Backtrack from current to neighbor to extract the loop
                cycle.clear();
                cycle.push_back(neighbor);
                int curr = current;
                while (curr != neighbor && curr != 0) {
                    cycle.push_back(curr);
                    curr = parent[curr];
                }
                cycle.push_back(neighbor);
                std::reverse(cycle.begin(), cycle.end());
                return true;
            }

            if (colors[neighbor] == NodeColor::WHITE) {
                parent[neighbor] = current;
                if (dfs(neighbor, adj, colors, parent, cycle)) {
                    return true;
                }
            }
        }
    }

    colors[current] = NodeColor::BLACK;
    return false;
}

bool CycleDetector::detectCycle(const WaitForGraph& graph, std::vector<int>& outCycle) {
    outCycle.clear();
    const auto& adj = graph.getAdjacencyList();
    std::set<int> allNodes = graph.getAllThreads();

    std::map<int, NodeColor> colors;
    std::map<int, int> parent;

    for (int node : allNodes) {
        colors[node] = NodeColor::WHITE;
        parent[node] = 0;
    }

    for (int node : allNodes) {
        if (colors[node] == NodeColor::WHITE) {
            if (dfs(node, adj, colors, parent, outCycle)) {
                return true;
            }
        }
    }

    return false;
}

} // namespace oscope
