#include "monitoring/resource_monitor.hpp"
#include "common/utils.hpp"
#include <sstream>

namespace oscope {

ResourceMonitor::ResourceMonitor(const Config& config) : config_(config) {}

std::vector<ResourceEventRecord> ResourceMonitor::evaluateProcessResources(const ProcessInfo& proc) {
    std::vector<ResourceEventRecord> events;
    const auto& m = proc.currentMetrics;

    // 1. Thread Count Threshold
    if (m.threadCount >= config_.threadCountThreshold) {
        ResourceEventRecord ev;
        ev.eventType = "THRESHOLD_EXCEEDED";
        ev.resourceType = "THREADS";
        ev.value = static_cast<double>(m.threadCount);
        ev.threshold = static_cast<double>(config_.threadCountThreshold);
        ev.severity = (m.threadCount >= config_.threadCountThreshold * 2) ? "CRITICAL" : "WARNING";
        ev.message = "Process " + proc.identity.processName + " (PID " + std::to_string(proc.identity.pid) +
                     ") thread count " + std::to_string(m.threadCount) + " exceeded threshold " +
                     std::to_string(config_.threadCountThreshold);
        events.push_back(ev);
    }

    // 2. Open File Descriptors Threshold
    if (m.openFileDescriptors >= config_.fdCountThreshold) {
        ResourceEventRecord ev;
        ev.eventType = "THRESHOLD_EXCEEDED";
        ev.resourceType = "FILE_DESCRIPTORS";
        ev.value = static_cast<double>(m.openFileDescriptors);
        ev.threshold = static_cast<double>(config_.fdCountThreshold);
        ev.severity = (m.openFileDescriptors >= config_.fdCountThreshold * 2) ? "CRITICAL" : "WARNING";
        ev.message = "Process " + proc.identity.processName + " (PID " + std::to_string(proc.identity.pid) +
                     ") open FD count " + std::to_string(m.openFileDescriptors) + " exceeded threshold " +
                     std::to_string(config_.fdCountThreshold);
        events.push_back(ev);
    }

    // 3. High CPU Spikes
    if (m.cpuUsagePercent >= config_.cpuPercentThreshold) {
        ResourceEventRecord ev;
        ev.eventType = "THRESHOLD_EXCEEDED";
        ev.resourceType = "CPU";
        ev.value = m.cpuUsagePercent;
        ev.threshold = config_.cpuPercentThreshold;
        ev.severity = (m.cpuUsagePercent >= 95.0) ? "CRITICAL" : "WARNING";
        ev.message = "Process " + proc.identity.processName + " (PID " + std::to_string(proc.identity.pid) +
                     ") CPU " + std::to_string(static_cast<int>(m.cpuUsagePercent)) + "% exceeded threshold " +
                     std::to_string(static_cast<int>(config_.cpuPercentThreshold)) + "%";
        events.push_back(ev);
    }

    return events;
}

std::vector<ResourceEventRecord> ResourceMonitor::evaluateSystemResources(const SystemMetrics& sys) {
    std::vector<ResourceEventRecord> events;

    // 1. System Memory Exhaustion
    if (sys.memoryPercent >= config_.memoryPercentThreshold) {
        ResourceEventRecord ev;
        ev.eventType = "THRESHOLD_EXCEEDED";
        ev.resourceType = "MEMORY";
        ev.value = sys.memoryPercent;
        ev.threshold = config_.memoryPercentThreshold;
        ev.severity = (sys.memoryPercent >= 90.0) ? "CRITICAL" : "WARNING";
        ev.message = "System memory usage " + std::to_string(static_cast<int>(sys.memoryPercent)) +
                     "% exceeded threshold " + std::to_string(static_cast<int>(config_.memoryPercentThreshold)) + "%";
        events.push_back(ev);
    }

    // 2. High Swap Usage
    if (sys.swapTotalMb > 0 && (sys.swapUsedMb / sys.swapTotalMb) >= 0.70) {
        double swapPct = (sys.swapUsedMb / sys.swapTotalMb) * 100.0;
        ResourceEventRecord ev;
        ev.eventType = "THRESHOLD_EXCEEDED";
        ev.resourceType = "SWAP";
        ev.value = swapPct;
        ev.threshold = 70.0;
        ev.severity = "WARNING";
        ev.message = "System swap utilization at " + std::to_string(static_cast<int>(swapPct)) + "%";
        events.push_back(ev);
    }

    return events;
}

} // namespace oscope
