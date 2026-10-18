#include "detection/statistical_detector.hpp"
#include <numeric>

namespace oscope {

StatisticalDetector::StatisticalDetector(int windowSize, double stdDevMultiplier)
    : windowSize_(windowSize), stdDevMultiplier_(stdDevMultiplier) {}

bool StatisticalDetector::addObservation(int pid, const std::string& metricName, double value, double& outMean, double& outStdDev) {
    std::string key = std::to_string(pid) + ":" + metricName;
    auto& window = historyWindows_[key];

    outMean = 0.0;
    outStdDev = 0.0;

    // Need a minimum baseline before statistical testing is valid
    if (window.size() < 4) {
        window.push_back(value);
        if (window.size() > static_cast<size_t>(windowSize_)) {
            window.pop_front();
        }
        return false;
    }

    // Calculate rolling mean
    double sum = std::accumulate(window.begin(), window.end(), 0.0);
    outMean = sum / window.size();

    // Calculate sample standard deviation
    double accum = 0.0;
    for (double x : window) {
        accum += (x - outMean) * (x - outMean);
    }
    outStdDev = std::sqrt(accum / window.size());

    // Update window
    window.push_back(value);
    if (window.size() > static_cast<size_t>(windowSize_)) {
        window.pop_front();
    }

    // Check statistical upper bound using effective stddev
    double effectiveStdDev = (outStdDev < 0.1) ? 1.0 : outStdDev;
    if (value > (outMean + (stdDevMultiplier_ * effectiveStdDev))) {
        return true;
    }

    return false;
}

void StatisticalDetector::clearProcess(int pid) {
    std::string prefix = std::to_string(pid) + ":";
    auto it = historyWindows_.begin();
    while (it != historyWindows_.end()) {
        if (it->first.rfind(prefix, 0) == 0) {
            it = historyWindows_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace oscope
