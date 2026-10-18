#pragma once

#include "common/types.hpp"
#include "common/config.hpp"
#include "detection/threshold_detector.hpp"
#include "detection/persistence_detector.hpp"
#include "detection/statistical_detector.hpp"
#include "deadlock/deadlock_detector.hpp"
#include <vector>
#include <map>
#include <string>
#include <set>

namespace oscope {

class DetectionEngine {
public:
    explicit DetectionEngine(const Config& config);

    // Analyzes a single process snapshot and returns newly detected incidents
    std::vector<IncidentRecord> analyzeProcess(const ProcessInfo& proc);

    // Analyzes an unexpected termination event
    std::vector<IncidentRecord> analyzeProcessTermination(int pid, const std::string& procName,
                                                          int exitCode, int termSignal,
                                                          int crashCount, bool isIntentionalStop);

    // Analyzes deadlock state
    std::vector<IncidentRecord> analyzeDeadlock(const DeadlockGraph& graph, int leaderPid = 0);

    // Marks an incident resolved or closed, clearing deduplication locks
    void resolveIncident(int pid, IncidentType type);

    // Checks if an active unresolved incident already exists
    bool hasActiveIncident(int pid, IncidentType type) const;

    // Cleans state for a removed process
    void clearProcess(int pid);

    ThresholdDetector& getThresholdDetector() { return thresholdDetector_; }
    PersistenceDetector& getPersistenceDetector() { return persistenceDetector_; }
    StatisticalDetector& getStatisticalDetector() { return statisticalDetector_; }

private:
    Config config_;
    ThresholdDetector thresholdDetector_;
    PersistenceDetector persistenceDetector_;
    StatisticalDetector statisticalDetector_;

    // Deduplication map: "pid:INCIDENT_TYPE" -> true if active incident open
    std::set<std::string> activeIncidents_;

    std::string makeIncidentKey(int pid, IncidentType type) const;
};

} // namespace oscope
