#pragma once

#include "common/types.hpp"
#include "monitoring/cpu_calculator.hpp"
#include "monitoring/memory_monitor.hpp"
#include <vector>
#include <map>
#include <memory>

namespace oscope {

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor() = default;

    // Discovers and collects process information for a specific PID from /proc
    bool readProcessDetails(int pid, ProcessInfo& outInfo);

    // Enumerates all currently active processes from /proc
    std::vector<int> discoverActivePids();

    // Collects snapshots for all active processes or tracked registered processes
    std::vector<ProcessInfo> scanAllProcesses();

    // Refreshes metrics for a specific tracked process
    bool refreshProcess(ProcessInfo& proc);

    CpuCalculator& getCpuCalculator() { return cpuCalculator_; }
    MemoryMonitor& getMemoryMonitor() { return memoryMonitor_; }

private:
    CpuCalculator cpuCalculator_;
    MemoryMonitor memoryMonitor_;

    // Helper functions for reading specific /proc files
    bool parseProcStatus(int pid, ProcessInfo& info);
    bool parseProcStat(int pid, ProcessInfo& info);
    bool parseProcIo(int pid, ProcessMetrics& metrics);
    int countOpenFds(int pid);
    std::string readCmdLine(int pid);
};

} // namespace oscope
