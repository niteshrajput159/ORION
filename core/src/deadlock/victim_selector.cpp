#include "deadlock/victim_selector.hpp"
#include <climits>
#include <algorithm>

namespace oscope {

VictimSelector::Strategy VictimSelector::stringToStrategy(const std::string& str) {
    if (str == "LOWEST_PRIORITY") return Strategy::LOWEST_PRIORITY;
    if (str == "HIGHEST_RESOURCE") return Strategy::HIGHEST_RESOURCE;
    return Strategy::LEAST_COST;
}

int VictimSelector::selectVictim(const std::vector<int>& cycleThreads,
                                const WaitForGraph& graph,
                                Strategy strategy,
                                const std::map<int, int>& threadPriorities) {
    if (cycleThreads.empty()) return 0;

    // Filter unique thread IDs in the cycle (since cycle starts and ends with same ID)
    std::set<int> uniqueCandidates(cycleThreads.begin(), cycleThreads.end());
    if (uniqueCandidates.empty()) return 0;

    int selectedVictim = *uniqueCandidates.begin();

    switch (strategy) {
        case Strategy::LOWEST_PRIORITY: {
            // In POSIX nice, higher value = lower scheduling priority
            int lowestPriorityValue = INT_MIN;
            for (int tid : uniqueCandidates) {
                int prio = 0;
                auto it = threadPriorities.find(tid);
                if (it != threadPriorities.end()) prio = it->second;
                if (prio > lowestPriorityValue) {
                    lowestPriorityValue = prio;
                    selectedVictim = tid;
                }
            }
            break;
        }

        case Strategy::HIGHEST_RESOURCE: {
            // Count number of outgoing edges / resources held
            std::map<int, int> resourcesHeld;
            for (const auto& edge : graph.getEdges()) {
                resourcesHeld[edge.holdingThreadId]++;
            }
            int maxHeld = -1;
            for (int tid : uniqueCandidates) {
                int count = resourcesHeld[tid];
                if (count > maxHeld) {
                    maxHeld = count;
                    selectedVictim = tid;
                }
            }
            break;
        }

        case Strategy::LEAST_COST:
        default: {
            // Choose thread holding fewer resources to minimize roll-back cost
            std::map<int, int> resourcesHeld;
            for (const auto& edge : graph.getEdges()) {
                resourcesHeld[edge.holdingThreadId]++;
            }
            int minHeld = INT_MAX;
            for (int tid : uniqueCandidates) {
                int count = resourcesHeld[tid];
                if (count < minHeld) {
                    minHeld = count;
                    selectedVictim = tid;
                }
            }
            break;
        }
    }

    return selectedVictim;
}

} // namespace oscope
