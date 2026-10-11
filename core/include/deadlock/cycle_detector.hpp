#pragma once

#include "deadlock/wait_for_graph.hpp"
#include <vector>
#include <map>

namespace oscope {

class CycleDetector {
public:
    CycleDetector() = default;

    // Detects whether a directed cycle exists in the Wait-For Graph.
    // If a cycle exists, outCycle will contain the ordered list of thread IDs: [T1, T2, ..., T1].
    bool detectCycle(const WaitForGraph& graph, std::vector<int>& outCycle);

private:
    enum class NodeColor {
        WHITE, // Unvisited
        GRAY,  // Currently in recursion stack
        BLACK  // Finished processing
    };

    bool dfs(int current,
             const std::map<int, std::vector<int>>& adj,
             std::map<int, NodeColor>& colors,
             std::map<int, int>& parent,
             std::vector<int>& cycle);
};

} // namespace oscope
