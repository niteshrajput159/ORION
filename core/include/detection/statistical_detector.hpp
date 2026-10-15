#pragma once

#include <deque>
#include <map>
#include <string>
#include <cmath>

namespace oscope {

class StatisticalDetector {
public:
    explicit StatisticalDetector(int windowSize = 10, double stdDevMultiplier = 2.5);

    // Adds a numeric observation to the sliding window for a process/metric.
    // Returns true if the value is a statistical anomaly (value > mean + multiplier * stddev).
    bool addObservation(int pid, const std::string& metricName, double value, double& outMean, double& outStdDev);

    void clearProcess(int pid);

    int getWindowSize() const { return windowSize_; }
    void setWindowSize(int size) { windowSize_ = size; }

    double getMultiplier() const { return stdDevMultiplier_; }
    void setMultiplier(double mult) { stdDevMultiplier_ = mult; }

private:
    int windowSize_{10};
    double stdDevMultiplier_{2.5};

    // Key: "pid:metricName" -> sliding window of observations
    std::map<std::string, std::deque<double>> historyWindows_;
};

} // namespace oscope
