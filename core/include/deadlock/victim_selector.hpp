#pragma once

#include "common/types.hpp"
#include "deadlock/wait_for_graph.hpp"
#include <string>
#include <vector>
#include <map>

namespace oscope {

class VictimSelector {
public:
    enum class Strategy {
        LEAST_COST,
        LOWEST_PRIORITY,
        HIGHEST_RESOURCE
    };

    static Strategy stringToStrategy(const std::string& str);

    // Selects the best victim thread from a detected cycle based on chosen strategy
    static int selectVictim(const std::vector<int>& cycleThreads,
                            const WaitForGraph& graph,
                            Strategy strategy,
                            const std::map<int, int>& threadPriorities = {});
};

} // namespace oscope
