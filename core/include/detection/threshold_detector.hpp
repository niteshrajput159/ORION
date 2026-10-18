#pragma once

#include "common/types.hpp"
#include "common/config.hpp"

namespace oscope {

class ThresholdDetector {
public:
    explicit ThresholdDetector(const Config& config);

    bool checkCpu(double cpuPercent) const;
    bool checkMemory(double memoryPercent) const;
    bool checkThreads(int threadCount) const;
    bool checkFds(int fdCount) const;
    bool checkDiskIo(double ioKbps) const;

    const Config& getConfig() const { return config_; }

private:
    Config config_;
};

} // namespace oscope
