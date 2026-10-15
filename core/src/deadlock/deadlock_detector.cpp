#include "deadlock/deadlock_detector.hpp"
#include "common/utils.hpp"
#include <fstream>
#include <sstream>

namespace oscope {

DeadlockDetector::DeadlockDetector(const std::string& defaultStrategy)
    : strategy_(defaultStrategy) {}

DeadlockGraph DeadlockDetector::analyzeGraph(WaitForGraph& graph) {
    DeadlockGraph result;
    result.strategyUsed = strategy_;

    graph.buildFromResourceAllocations();

    for (int tid : graph.getAllThreads()) {
        result.threadIds.push_back(tid);
    }
    result.edges = graph.getEdges();

    std::vector<int> cycle;
    if (cycleDetector_.detectCycle(graph, cycle)) {
        result.hasDeadlock = true;
        result.cycleThreadIds = cycle;

        auto stratEnum = VictimSelector::stringToStrategy(strategy_);
        result.victimThreadId = VictimSelector::selectVictim(cycle, graph, stratEnum);
    } else {
        result.hasDeadlock = false;
        result.victimThreadId = 0;
    }

    return result;
}

DeadlockGraph DeadlockDetector::analyzeFromStateFile(const std::string& stateFilePath) {
    WaitForGraph graph;
    DeadlockGraph emptyResult;
    emptyResult.hasDeadlock = false;
    emptyResult.strategyUsed = strategy_;

    std::ifstream file(stateFilePath);
    if (!file.is_open()) {
        return emptyResult;
    }

    int foundPid = 0;
    std::string line;
    while (std::getline(file, line)) {
        line = utils::trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string actionType;
        int threadId = 0;
        std::string resourceName;

        ss >> actionType >> threadId >> resourceName;
        if (actionType == "PID" && threadId > 0) {
            foundPid = threadId;
            continue;
        }
        if (threadId > 0 && !resourceName.empty()) {
            if (actionType == "HOLD") {
                graph.setResourceHolder(resourceName, threadId);
            } else if (actionType == "WAIT") {
                graph.setResourceRequest(resourceName, threadId);
            }
        }
    }

    DeadlockGraph result = analyzeGraph(graph);
    result.processPid = foundPid;
    return result;
}

} // namespace oscope
