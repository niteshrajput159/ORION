#pragma once

#include "common/types.hpp"
#include "common/config.hpp"
#include <vector>
#include <string>

namespace oscope {

class ResourceMonitor {
public:
    explicit ResourceMonitor(const Config& config);

    // Evaluates process metrics against resource thresholds
    std::vector<ResourceEventRecord> evaluateProcessResources(const ProcessInfo& proc);

    // Evaluates system-wide metrics against thresholds
    std::vector<ResourceEventRecord> evaluateSystemResources(const SystemMetrics& sys);

private:
    Config config_;
};

} // namespace oscope
