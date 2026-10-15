#pragma once

#include "common/types.hpp"
#include <map>
#include <string>

namespace oscope {

class PersistenceDetector {
public:
    PersistenceDetector() = default;

    // Registers a sample outcome (true if exceeding threshold, false if normal).
    // Returns true ONLY when the condition has persisted for requiredConsecutive samples.
    bool recordSample(int pid, const std::string& metricName, bool isViolating, int requiredConsecutive);

    // Clears state for a terminated process
    void clearProcess(int pid);

    // Retrieves current consecutive count
    int getConsecutiveCount(int pid, const std::string& metricName) const;

private:
    // Key: "pid:metricName" -> consecutive violating samples count
    std::map<std::string, int> violationCounters_;
};

} // namespace oscope
