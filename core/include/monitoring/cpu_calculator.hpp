#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <chrono>

namespace oscope {

/**
 * CpuCalculator:
 * Accurately calculates process and system CPU utilization without fake numbers.
 * 
 * Mathematical Formulation on Linux:
 * 1. Total System CPU Time (from /proc/stat):
 *    T_sys = user + nice + system + idle + iowait + irq + softirq + steal
 * 2. Process CPU Time (from /proc/[pid]/stat):
 *    T_proc = utime (field 14) + stime (field 15)
 * 3. Delta calculation over sampling interval [t1, t2]:
 *    ΔT_proc = T_proc(t2) - T_proc(t1)
 *    ΔT_sys  = T_sys(t2) - T_sys(t1)
 * 4. Process CPU Usage Percentage:
 *    CPU% = (ΔT_proc / ΔT_sys) * 100.0 * N_cores
 *    Or via elapsed wall-clock seconds Δt:
 *    CPU% = ((ΔT_proc / CLK_TCK) / Δt) * 100.0
 */
class CpuCalculator {
public:
    struct CpuSample {
        uint64_t utimeTicks{0};
        uint64_t stimeTicks{0};
        uint64_t systemTotalTicks{0};
        uint64_t timestampMs{0};
    };

    CpuCalculator();

    // Reads current total system CPU ticks from /proc/stat
    static uint64_t readSystemTotalCpuTicks();

    // Reads process user and kernel ticks from /proc/[pid]/stat
    static bool readProcessCpuTicks(int pid, uint64_t& outUtime, uint64_t& outStime);

    // Calculates process CPU usage % based on previous sample and current reading
    double calculateProcessCpu(int pid);

    // Calculates total system CPU usage %
    double calculateSystemCpu();

    // Resets tracking for a process (e.g. when process terminates)
    void removeProcess(int pid);

    void clearAll();

private:
    std::map<int, CpuSample> prevProcessSamples_;
    uint64_t prevSystemTotalTicks_{0};
    uint64_t prevSystemIdleTicks_{0};
    int numCpus_{1};
    long clockTicksPerSec_{100};
};

} // namespace oscope
