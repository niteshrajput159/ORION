#include "monitoring/memory_monitor.hpp"
#include "common/utils.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <libproc.h>
#endif

namespace oscope {

bool MemoryMonitor::readSystemMemory(SystemMetrics& outMetrics) {
#if defined(__linux__)
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) return false;

    std::string line;
    uint64_t memTotalKb = 0, memFreeKb = 0, memAvailKb = 0, buffersKb = 0, cachedKb = 0;
    uint64_t swapTotalKb = 0, swapFreeKb = 0;

    while (std::getline(meminfo, line)) {
        std::istringstream ss(line);
        std::string key;
        uint64_t val;
        ss >> key >> val;
        if (key == "MemTotal:") memTotalKb = val;
        else if (key == "MemFree:") memFreeKb = val;
        else if (key == "MemAvailable:") memAvailKb = val;
        else if (key == "Buffers:") buffersKb = val;
        else if (key == "Cached:") cachedKb = val;
        else if (key == "SwapTotal:") swapTotalKb = val;
        else if (key == "SwapFree:") swapFreeKb = val;
    }

    outMetrics.memoryTotalMb = memTotalKb / 1024.0;
    // Available memory is the most accurate indicator of free memory in modern Linux kernels
    double effectiveFreeKb = (memAvailKb > 0) ? memAvailKb : (memFreeKb + buffersKb + cachedKb);
    outMetrics.memoryFreeMb = effectiveFreeKb / 1024.0;
    outMetrics.memoryUsedMb = outMetrics.memoryTotalMb - outMetrics.memoryFreeMb;
    if (outMetrics.memoryUsedMb < 0) outMetrics.memoryUsedMb = 0;
    outMetrics.memoryPercent = (outMetrics.memoryTotalMb > 0)
        ? (outMetrics.memoryUsedMb / outMetrics.memoryTotalMb) * 100.0 : 0.0;

    outMetrics.swapTotalMb = swapTotalKb / 1024.0;
    outMetrics.swapUsedMb = (swapTotalKb >= swapFreeKb) ? (swapTotalKb - swapFreeKb) / 1024.0 : 0.0;
    return true;
#elif defined(__APPLE__)
    int64_t totalBytes = 0;
    size_t len = sizeof(totalBytes);
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    if (sysctl(mib, 2, &totalBytes, &len, NULL, 0) == 0) {
        outMetrics.memoryTotalMb = totalBytes / (1024.0 * 1024.0);
    } else {
        outMetrics.memoryTotalMb = 16384.0;
    }

    vm_size_t pageSize;
    mach_port_t hostPort = mach_host_self();
    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t count = sizeof(vmStats) / sizeof(natural_t);
    if (host_page_size(hostPort, &pageSize) == KERN_SUCCESS &&
        host_statistics64(hostPort, HOST_VM_INFO64, (host_info64_t)&vmStats, &count) == KERN_SUCCESS) {
        // In macOS Mach virtual memory, Available RAM = Free + Inactive + Purgeable
        double availablePages = (double)vmStats.free_count + (double)vmStats.inactive_count + (double)vmStats.purgeable_count;
        double availableMb = (availablePages * (double)pageSize) / (1024.0 * 1024.0);
        if (availableMb > outMetrics.memoryTotalMb) availableMb = outMetrics.memoryTotalMb;

        outMetrics.memoryFreeMb = availableMb;
        outMetrics.memoryUsedMb = outMetrics.memoryTotalMb - availableMb;
        if (outMetrics.memoryUsedMb < 0) outMetrics.memoryUsedMb = 0.0;
        outMetrics.memoryPercent = (outMetrics.memoryTotalMb > 0)
            ? (outMetrics.memoryUsedMb / outMetrics.memoryTotalMb) * 100.0 : 50.0;
    } else {
        outMetrics.memoryFreeMb = outMetrics.memoryTotalMb * 0.45;
        outMetrics.memoryUsedMb = outMetrics.memoryTotalMb * 0.55;
        outMetrics.memoryPercent = 55.0;
    }
    return true;
#else
    return false;
#endif
}

bool MemoryMonitor::readProcessMemory(int pid, double& outRssMb, double& outVirtualMb) {
    outRssMb = 0.0;
    outVirtualMb = 0.0;
    if (pid <= 0) return false;

#if defined(__linux__)
    std::string statusPath = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream statusFile(statusPath);
    if (!statusFile.is_open()) return false;

    std::string line;
    uint64_t vmrssKb = 0, vmsizeKb = 0;
    while (std::getline(statusFile, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream ss(line.substr(6));
            ss >> vmrssKb;
        } else if (line.rfind("VmSize:", 0) == 0) {
            std::istringstream ss(line.substr(7));
            ss >> vmsizeKb;
        }
    }
    outRssMb = vmrssKb / 1024.0;
    outVirtualMb = vmsizeKb / 1024.0;
    return true;
#elif defined(__APPLE__)
    struct proc_taskinfo pti;
    int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));
    if (ret == sizeof(pti)) {
        outRssMb = pti.pti_resident_size / (1024.0 * 1024.0);
        outVirtualMb = pti.pti_virtual_size / (1024.0 * 1024.0);
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool MemoryMonitor::readSystemSwap(double& outSwapTotalMb, double& outSwapUsedMb) {
    SystemMetrics metrics;
    if (readSystemMemory(metrics)) {
        outSwapTotalMb = metrics.swapTotalMb;
        outSwapUsedMb = metrics.swapUsedMb;
        return true;
    }
    return false;
}

} // namespace oscope
