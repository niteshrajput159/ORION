#include "monitoring/cpu_calculator.hpp"
#include "common/utils.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <libproc.h>
#include <sys/sysctl.h>
#endif

namespace oscope {

CpuCalculator::CpuCalculator() {
    numCpus_ = utils::getNumCpus();
    clockTicksPerSec_ = utils::getClockTicksPerSecond();
}

uint64_t CpuCalculator::readSystemTotalCpuTicks() {
#if defined(__linux__)
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) return 0;

    std::string line;
    if (std::getline(statFile, line)) {
        std::istringstream ss(line);
        std::string cpuLabel;
        uint64_t user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
        ss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    return 0;
#elif defined(__APPLE__)
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        return cpuinfo.cpu_ticks[CPU_STATE_USER] + cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] +
               cpuinfo.cpu_ticks[CPU_STATE_IDLE] + cpuinfo.cpu_ticks[CPU_STATE_NICE];
    }
    return 0;
#else
    return 0;
#endif
}

bool CpuCalculator::readProcessCpuTicks(int pid, uint64_t& outUtime, uint64_t& outStime) {
    outUtime = 0;
    outStime = 0;
    if (pid <= 0) return false;

#if defined(__linux__)
    std::string statPath = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream statFile(statPath);
    if (!statFile.is_open()) return false;

    std::string content;
    std::getline(statFile, content);
    if (content.empty()) return false;

    // Find the closing parenthesis for comm (command name), to safely handle spaces in process names
    size_t lastParen = content.rfind(')');
    if (lastParen == std::string::npos || lastParen + 2 >= content.length()) {
        return false;
    }

    // Remainder of string starts with: ' <state> <ppid> ...'
    // Field 3 in stat is state, Field 14 is utime, Field 15 is stime.
    // Relative to lastParen + 2:
    // index 0: state (field 3)
    // index 1: ppid (field 4)
    // ...
    // index 11: utime (field 14)
    // index 12: stime (field 15)
    std::string rest = content.substr(lastParen + 2);
    std::istringstream ss(rest);
    std::vector<std::string> tokens;
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() >= 13) {
        try {
            outUtime = std::stoull(tokens[11]);
            outStime = std::stoull(tokens[12]);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
#elif defined(__APPLE__)
    struct proc_taskinfo pti;
    int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));
    if (ret == sizeof(pti)) {
        // pti.pti_total_user and pti.pti_total_system are in nanoseconds
        outUtime = pti.pti_total_user / 10000000ULL; // convert to centiseconds (100 ticks/sec)
        outStime = pti.pti_total_system / 10000000ULL;
        return true;
    }
    return false;
#else
    return false;
#endif
}

double CpuCalculator::calculateProcessCpu(int pid) {
    uint64_t utime = 0, stime = 0;
    if (!readProcessCpuTicks(pid, utime, stime)) {
        removeProcess(pid);
        return 0.0;
    }

    uint64_t sysTicks = readSystemTotalCpuTicks();
    uint64_t nowMs = utils::getMonotonicTimeMs();

    auto it = prevProcessSamples_.find(pid);
    if (it == prevProcessSamples_.end()) {
        // First sample: store baseline and return 0.0 (need 2 samples for delta)
        prevProcessSamples_[pid] = {utime, stime, sysTicks, nowMs};
        return 0.0;
    }

    CpuSample prev = it->second;
    it->second = {utime, stime, sysTicks, nowMs};

    uint64_t procTicksDelta = 0;
    if ((utime + stime) >= (prev.utimeTicks + prev.stimeTicks)) {
        procTicksDelta = (utime + stime) - (prev.utimeTicks + prev.stimeTicks);
    }

    double elapsedSeconds = (nowMs - prev.timestampMs) / 1000.0;
    if (elapsedSeconds <= 0.001) {
        return 0.0;
    }

    // Method: process CPU seconds divided by wall-clock elapsed time
    double procCpuSeconds = static_cast<double>(procTicksDelta) / static_cast<double>(clockTicksPerSec_);
    double cpuPercent = (procCpuSeconds / elapsedSeconds) * 100.0;

    // Sanity clamp: 0.0% to 100.0% * numCpus_
    if (cpuPercent < 0.0) cpuPercent = 0.0;
    if (cpuPercent > (100.0 * numCpus_)) cpuPercent = 100.0 * numCpus_;

    return cpuPercent;
}

double CpuCalculator::calculateSystemCpu() {
#if defined(__linux__)
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) return 0.0;

    std::string line;
    if (!std::getline(statFile, line)) return 0.0;

    std::istringstream ss(line);
    std::string cpuLabel;
    uint64_t user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
    ss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    uint64_t currentTotal = user + nice + system + idle + iowait + irq + softirq + steal;
    uint64_t currentIdle = idle + iowait;

    if (prevSystemTotalTicks_ == 0) {
        prevSystemTotalTicks_ = currentTotal;
        prevSystemIdleTicks_ = currentIdle;
        return 0.0;
    }

    uint64_t totalDelta = currentTotal - prevSystemTotalTicks_;
    uint64_t idleDelta = (currentIdle >= prevSystemIdleTicks_) ? (currentIdle - prevSystemIdleTicks_) : 0;

    prevSystemTotalTicks_ = currentTotal;
    prevSystemIdleTicks_ = currentIdle;

    if (totalDelta == 0) return 0.0;

    double busyTicks = static_cast<double>(totalDelta - idleDelta);
    double usagePercent = (busyTicks / static_cast<double>(totalDelta)) * 100.0;
    return std::max(0.0, std::min(100.0, usagePercent));
#elif defined(__APPLE__)
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        uint64_t total = cpuinfo.cpu_ticks[CPU_STATE_USER] + cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] +
                         cpuinfo.cpu_ticks[CPU_STATE_IDLE] + cpuinfo.cpu_ticks[CPU_STATE_NICE];
        uint64_t idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];

        if (prevSystemTotalTicks_ == 0) {
            prevSystemTotalTicks_ = total;
            prevSystemIdleTicks_ = idle;
            return 0.0;
        }

        uint64_t totalDelta = total - prevSystemTotalTicks_;
        uint64_t idleDelta = (idle >= prevSystemIdleTicks_) ? (idle - prevSystemIdleTicks_) : 0;
        prevSystemTotalTicks_ = total;
        prevSystemIdleTicks_ = idle;

        if (totalDelta == 0) return 0.0;
        double usage = (static_cast<double>(totalDelta - idleDelta) / totalDelta) * 100.0;
        return std::max(0.0, std::min(100.0, usage));
    }
    return 0.0;
#else
    return 0.0;
#endif
}

void CpuCalculator::removeProcess(int pid) {
    prevProcessSamples_.erase(pid);
}

void CpuCalculator::clearAll() {
    prevProcessSamples_.clear();
}

} // namespace oscope
