#include "detection/persistence_detector.hpp"

namespace oscope {

bool PersistenceDetector::recordSample(int pid, const std::string& metricName, bool isViolating, int requiredConsecutive) {
    std::string key = std::to_string(pid) + ":" + metricName;

    if (!isViolating) {
        // Condition returned to normal: reset counter
        violationCounters_[key] = 0;
        return false;
    }

    int& currentCount = violationCounters_[key];
    currentCount++;

    if (currentCount >= requiredConsecutive) {
        return true;
    }

    return false;
}

void PersistenceDetector::clearProcess(int pid) {
    std::string prefix = std::to_string(pid) + ":";
    auto it = violationCounters_.begin();
    while (it != violationCounters_.end()) {
        if (it->first.rfind(prefix, 0) == 0) {
            it = violationCounters_.erase(it);
        } else {
            ++it;
        }
    }
}

int PersistenceDetector::getConsecutiveCount(int pid, const std::string& metricName) const {
    std::string key = std::to_string(pid) + ":" + metricName;
    auto it = violationCounters_.find(key);
    return (it != violationCounters_.end()) ? it->second : 0;
}

} // namespace oscope
