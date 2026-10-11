#pragma once

#include "common/types.hpp"
#include <map>
#include <set>
#include <vector>
#include <string>

namespace oscope {

class WaitForGraph {
public:
    WaitForGraph() = default;

    // Adds a directed dependency edge: waitingThreadId -> holdingThreadId on resource
    void addEdge(int waitingThreadId, int holdingThreadId, const std::string& resource);

    // Records a thread holding a resource
    void setResourceHolder(const std::string& resource, int holdingThreadId);

    // Records a thread requesting/waiting for a resource
    void setResourceRequest(const std::string& resource, int waitingThreadId);

    // Constructs the wait-for edges from registered resource hold and wait mappings
    void buildFromResourceAllocations();

    const std::map<int, std::vector<int>>& getAdjacencyList() const { return adj_; }
    const std::vector<DeadlockEdge>& getEdges() const { return edges_; }
    std::set<int> getAllThreads() const;

    void clear();

private:
    std::map<int, std::vector<int>> adj_; // waitingThread -> [holdingThreads]
    std::vector<DeadlockEdge> edges_;
    std::map<std::string, int> resourceHolders_; // resource -> threadId
    std::map<std::string, std::vector<int>> resourceWaiters_; // resource -> [waitingThreadIds]
};

} // namespace oscope
