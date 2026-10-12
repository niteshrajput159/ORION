#pragma once

#include "common/types.hpp"
#include <string>

namespace oscope {

class MemoryMonitor {
public:
    MemoryMonitor() = default;

    // Reads system-wide memory from /proc/meminfo
    bool readSystemMemory(SystemMetrics& outMetrics);

    // Reads process memory from /proc/[pid]/status or /proc/[pid]/statm
    bool readProcessMemory(int pid, double& outRssMb, double& outVirtualMb);

    // Reads swap info
    bool readSystemSwap(double& outSwapTotalMb, double& outSwapUsedMb);
};

} // namespace oscope
