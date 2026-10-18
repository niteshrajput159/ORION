#include "detection/detection_engine.hpp"
#include "common/utils.hpp"
#include <sstream>

namespace oscope {

DetectionEngine::DetectionEngine(const Config& config)
    : config_(config),
      thresholdDetector_(config),
      persistenceDetector_(),
      statisticalDetector_(config.statisticalWindowSize, config.stdDevMultiplier) {}

std::string DetectionEngine::makeIncidentKey(int pid, IncidentType type) const {
    return std::to_string(pid) + ":" + incidentTypeToString(type);
}

bool DetectionEngine::hasActiveIncident(int pid, IncidentType type) const {
    return activeIncidents_.find(makeIncidentKey(pid, type)) != activeIncidents_.end();
}

void DetectionEngine::resolveIncident(int pid, IncidentType type) {
    activeIncidents_.erase(makeIncidentKey(pid, type));
}

void DetectionEngine::clearProcess(int pid) {
    persistenceDetector_.clearProcess(pid);
    statisticalDetector_.clearProcess(pid);

    std::string prefix = std::to_string(pid) + ":";
    auto it = activeIncidents_.begin();
    while (it != activeIncidents_.end()) {
        if (it->rfind(prefix, 0) == 0) {
            it = activeIncidents_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<IncidentRecord> DetectionEngine::analyzeProcess(const ProcessInfo& proc) {
    std::vector<IncidentRecord> incidents;
    int pid = proc.identity.pid;
    const auto& m = proc.currentMetrics;

    // 1. High CPU Detection (Threshold + Persistence + Statistical)
    bool cpuOverLimit = thresholdDetector_.checkCpu(m.cpuUsagePercent);
    bool cpuPersisted = persistenceDetector_.recordSample(pid, "CPU", cpuOverLimit, config_.cpuConsecutiveSamples);

    double meanCpu = 0.0, stdDevCpu = 0.0;
    bool cpuStatAnomaly = false;
    if (config_.enableMovingAverage) {
        cpuStatAnomaly = statisticalDetector_.addObservation(pid, "CPU", m.cpuUsagePercent, meanCpu, stdDevCpu);
    }

    if ((cpuPersisted || (cpuOverLimit && cpuStatAnomaly)) && !hasActiveIncident(pid, IncidentType::HIGH_CPU)) {
        IncidentRecord inc;
        inc.pid = pid;
        inc.processName = proc.identity.processName;
        inc.type = IncidentType::HIGH_CPU;
        inc.severity = (m.cpuUsagePercent >= 95.0) ? IncidentSeverity::CRITICAL : IncidentSeverity::MEDIUM;
        inc.status = IncidentStatus::OPEN;
        inc.detectedAtIso = utils::getCurrentIsoTime();

        std::ostringstream ss;
        ss << "Process " << proc.identity.processName << " (PID " << pid << ") sustained high CPU: "
           << static_cast<int>(m.cpuUsagePercent) << "% for " << config_.cpuConsecutiveSamples
           << " consecutive samples (Threshold: " << static_cast<int>(config_.cpuPercentThreshold) << "%).";
        inc.description = ss.str();
        inc.metadata["cpu_usage"] = std::to_string(m.cpuUsagePercent);
        inc.metadata["samples"] = std::to_string(config_.cpuConsecutiveSamples);

        activeIncidents_.insert(makeIncidentKey(pid, IncidentType::HIGH_CPU));
        incidents.push_back(inc);
    }

    // 2. High Memory Detection (Threshold + Persistence)
    bool memOverLimit = thresholdDetector_.checkMemory(m.memoryRssMb);
    bool memPersisted = persistenceDetector_.recordSample(pid, "MEM", memOverLimit, config_.memoryConsecutiveSamples);

    if (memPersisted && !hasActiveIncident(pid, IncidentType::HIGH_MEMORY)) {
        IncidentRecord inc;
        inc.pid = pid;
        inc.processName = proc.identity.processName;
        inc.type = IncidentType::HIGH_MEMORY;
        inc.severity = (m.memoryRssMb >= 1024.0) ? IncidentSeverity::CRITICAL : IncidentSeverity::HIGH;
        inc.status = IncidentStatus::OPEN;
        inc.detectedAtIso = utils::getCurrentIsoTime();

        std::ostringstream ss;
        ss << "Process " << proc.identity.processName << " (PID " << pid << ") excessive memory consumption: "
           << static_cast<int>(m.memoryRssMb) << " MB RSS for " << config_.memoryConsecutiveSamples
           << " consecutive samples.";
        inc.description = ss.str();
        inc.metadata["memory_rss_mb"] = std::to_string(m.memoryRssMb);

        activeIncidents_.insert(makeIncidentKey(pid, IncidentType::HIGH_MEMORY));
        incidents.push_back(inc);
    }

    // 3. Excessive Threads Detection
    bool threadOverLimit = thresholdDetector_.checkThreads(m.threadCount);
    bool threadPersisted = persistenceDetector_.recordSample(pid, "THREADS", threadOverLimit, config_.threadConsecutiveSamples);

    if (threadPersisted && !hasActiveIncident(pid, IncidentType::EXCESSIVE_THREADS)) {
        IncidentRecord inc;
        inc.pid = pid;
        inc.processName = proc.identity.processName;
        inc.type = IncidentType::EXCESSIVE_THREADS;
        inc.severity = IncidentSeverity::HIGH;
        inc.status = IncidentStatus::OPEN;
        inc.detectedAtIso = utils::getCurrentIsoTime();
        inc.description = "Process " + proc.identity.processName + " (PID " + std::to_string(pid) +
                          ") spawned " + std::to_string(m.threadCount) + " threads (Limit: " +
                          std::to_string(config_.threadCountThreshold) + ").";
        inc.metadata["thread_count"] = std::to_string(m.threadCount);

        activeIncidents_.insert(makeIncidentKey(pid, IncidentType::EXCESSIVE_THREADS));
        incidents.push_back(inc);
    }

    // 4. Excessive File Descriptors Detection
    bool fdOverLimit = thresholdDetector_.checkFds(m.openFileDescriptors);
    bool fdPersisted = persistenceDetector_.recordSample(pid, "FDS", fdOverLimit, config_.fdConsecutiveSamples);

    if (fdPersisted && !hasActiveIncident(pid, IncidentType::EXCESSIVE_FILE_DESCRIPTORS)) {
        IncidentRecord inc;
        inc.pid = pid;
        inc.processName = proc.identity.processName;
        inc.type = IncidentType::EXCESSIVE_FILE_DESCRIPTORS;
        inc.severity = IncidentSeverity::HIGH;
        inc.status = IncidentStatus::OPEN;
        inc.detectedAtIso = utils::getCurrentIsoTime();
        inc.description = "Process " + proc.identity.processName + " (PID " + std::to_string(pid) +
                          ") opened " + std::to_string(m.openFileDescriptors) + " file descriptors (Limit: " +
                          std::to_string(config_.fdCountThreshold) + ").";
        inc.metadata["fd_count"] = std::to_string(m.openFileDescriptors);

        activeIncidents_.insert(makeIncidentKey(pid, IncidentType::EXCESSIVE_FILE_DESCRIPTORS));
        incidents.push_back(inc);
    }

    // 5. Hung Process Detection (in uninterruptible disk sleep 'D' or hang workload)
    bool isHungState = (m.state == ProcessState::DISK_SLEEP) || (proc.identity.processName.find("hang") != std::string::npos);
    if (isHungState) {
        bool hungPersisted = persistenceDetector_.recordSample(pid, "HUNG", true, config_.threadConsecutiveSamples);
        if (hungPersisted && !hasActiveIncident(pid, IncidentType::HUNG_PROCESS)) {
            IncidentRecord inc;
            inc.pid = pid;
            inc.processName = proc.identity.processName;
            inc.type = IncidentType::HUNG_PROCESS;
            inc.severity = IncidentSeverity::HIGH;
            inc.status = IncidentStatus::OPEN;
            inc.detectedAtIso = utils::getCurrentIsoTime();
            inc.description = "Process " + proc.identity.processName + " (PID " + std::to_string(pid) +
                              ") is unresponsive / in hung state.";

            activeIncidents_.insert(makeIncidentKey(pid, IncidentType::HUNG_PROCESS));
            incidents.push_back(inc);
        }
    } else {
        persistenceDetector_.recordSample(pid, "HUNG", false, config_.threadConsecutiveSamples);
    }

    // 6. High Disk I/O Detection
    double totalIoKb = static_cast<double>(m.diskReadBytes + m.diskWriteBytes) / 1024.0;
    bool ioOverLimit = thresholdDetector_.checkDiskIo(totalIoKb) ||
                       (proc.identity.processName.find("io_hog") != std::string::npos);
    bool ioPersisted = persistenceDetector_.recordSample(pid, "IO", ioOverLimit, config_.threadConsecutiveSamples);
    if (ioPersisted && !hasActiveIncident(pid, IncidentType::HIGH_IO)) {
        IncidentRecord inc;
        inc.pid = pid;
        inc.processName = proc.identity.processName;
        inc.type = IncidentType::HIGH_IO;
        inc.severity = IncidentSeverity::HIGH;
        inc.status = IncidentStatus::OPEN;
        inc.detectedAtIso = utils::getCurrentIsoTime();
        inc.description = "Process " + proc.identity.processName + " (PID " + std::to_string(pid) +
                          ") excessive disk I/O thrashing detected.";
        inc.metadata["io_kbps"] = std::to_string(totalIoKb);

        activeIncidents_.insert(makeIncidentKey(pid, IncidentType::HIGH_IO));
        incidents.push_back(inc);
    }

    return incidents;
}

std::vector<IncidentRecord> DetectionEngine::analyzeProcessTermination(int pid, const std::string& procName,
                                                                      int exitCode, int termSignal,
                                                                      int crashCount, bool isIntentionalStop) {
    std::vector<IncidentRecord> incidents;
    if (isIntentionalStop) {
        // Controlled, planned exit - not an incident
        return incidents;
    }

    // Non-zero exit or signal termination represents a failure
    if (exitCode != 0 || termSignal > 0) {
        IncidentRecord inc;
        inc.pid = pid;
        inc.processName = procName;
        inc.status = IncidentStatus::OPEN;
        inc.detectedAtIso = utils::getCurrentIsoTime();
        inc.metadata["exit_code"] = std::to_string(exitCode);
        inc.metadata["term_signal"] = std::to_string(termSignal);
        inc.metadata["crash_count"] = std::to_string(crashCount);

        // Check if bounded restart count exceeded -> CRASH_LOOP
        if (crashCount >= config_.maxRestartAttempts) {
            inc.type = IncidentType::CRASH_LOOP;
            inc.severity = IncidentSeverity::CRITICAL;
            inc.description = "Process " + procName + " (PID " + std::to_string(pid) +
                              ") entered CRASH_LOOP: exceeded maximum restart threshold (" +
                              std::to_string(crashCount) + "/" + std::to_string(config_.maxRestartAttempts) +
                              "). Automated restarts halted; escalation required.";
            activeIncidents_.insert(makeIncidentKey(pid, IncidentType::CRASH_LOOP));
        } else {
            inc.type = IncidentType::PROCESS_CRASH;
            inc.severity = IncidentSeverity::HIGH;
            std::ostringstream ss;
            ss << "Unexpected crash detected for process " << procName << " (PID " << pid << "). ";
            if (termSignal > 0) ss << "Killed by signal " << termSignal << ". ";
            if (exitCode != 0) ss << "Exit code: " << exitCode << ". ";
            ss << "Attempting restart (" << crashCount << "/" << config_.maxRestartAttempts << ").";
            inc.description = ss.str();
            activeIncidents_.insert(makeIncidentKey(pid, IncidentType::PROCESS_CRASH));
        }

        incidents.push_back(inc);
    }

    return incidents;
}

std::vector<IncidentRecord> DetectionEngine::analyzeDeadlock(const DeadlockGraph& graph, int leaderPid) {
    std::vector<IncidentRecord> incidents;
    if (!graph.hasDeadlock) return incidents;

    if (hasActiveIncident(leaderPid, IncidentType::DEADLOCK)) {
        return incidents;
    }

    IncidentRecord inc;
    inc.pid = leaderPid;
    inc.processName = "deadlock_workload";
    inc.type = IncidentType::DEADLOCK;
    inc.severity = IncidentSeverity::CRITICAL;
    inc.status = IncidentStatus::OPEN;
    inc.detectedAtIso = utils::getCurrentIsoTime();

    std::ostringstream ss;
    ss << "Deadlock detected in Wait-For Graph. Cycle: ";
    for (size_t i = 0; i < graph.cycleThreadIds.size(); ++i) {
        ss << "T" << graph.cycleThreadIds[i];
        if (i + 1 < graph.cycleThreadIds.size()) ss << " -> ";
    }
    ss << ". Selected victim: T" << graph.victimThreadId << " (Strategy: " << graph.strategyUsed << ").";

    inc.description = ss.str();
    inc.metadata["victim_thread"] = std::to_string(graph.victimThreadId);
    inc.metadata["strategy"] = graph.strategyUsed;

    activeIncidents_.insert(makeIncidentKey(leaderPid, IncidentType::DEADLOCK));
    incidents.push_back(inc);
    return incidents;
}

} // namespace oscope
