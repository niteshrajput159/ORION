#include "detection/threshold_detector.hpp"

namespace oscope {

ThresholdDetector::ThresholdDetector(const Config& config) : config_(config) {}

bool ThresholdDetector::checkCpu(double cpuPercent) const {
    return cpuPercent >= config_.cpuPercentThreshold;
}

bool ThresholdDetector::checkMemory(double memoryPercent) const {
    return memoryPercent >= config_.memoryPercentThreshold;
}

bool ThresholdDetector::checkThreads(int threadCount) const {
    return threadCount >= config_.threadCountThreshold;
}

bool ThresholdDetector::checkFds(int fdCount) const {
    return fdCount >= config_.fdCountThreshold;
}

bool ThresholdDetector::checkDiskIo(double ioKbps) const {
    return ioKbps >= config_.diskIoKbpsThreshold;
}

} // namespace oscope
