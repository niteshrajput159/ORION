#include "deadlock/wait_for_graph.hpp"

namespace oscope {

void WaitForGraph::addEdge(int waitingThreadId, int holdingThreadId, const std::string& resource) {
    adj_[waitingThreadId].push_back(holdingThreadId);
    edges_.push_back({waitingThreadId, holdingThreadId, resource});
}

void WaitForGraph::setResourceHolder(const std::string& resource, int holdingThreadId) {
    resourceHolders_[resource] = holdingThreadId;
}

void WaitForGraph::setResourceRequest(const std::string& resource, int waitingThreadId) {
    resourceWaiters_[resource].push_back(waitingThreadId);
}

void WaitForGraph::buildFromResourceAllocations() {
    adj_.clear();
    edges_.clear();

    for (const auto& req : resourceWaiters_) {
        const std::string& res = req.first;
        auto holderIt = resourceHolders_.find(res);
        if (holderIt != resourceHolders_.end()) {
            int holderThread = holderIt->second;
            for (int waiterThread : req.second) {
                if (waiterThread != holderThread) {
                    addEdge(waiterThread, holderThread, res);
                }
            }
        }
    }
}

std::set<int> WaitForGraph::getAllThreads() const {
    std::set<int> threads;
    for (const auto& pair : adj_) {
        threads.insert(pair.first);
        for (int neighbor : pair.second) {
            threads.insert(neighbor);
        }
    }
    return threads;
}

void WaitForGraph::clear() {
    adj_.clear();
    edges_.clear();
    resourceHolders_.clear();
    resourceWaiters_.clear();
}

} // namespace oscope
