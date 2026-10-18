#pragma once

#include "common/types.hpp"
#include <string>

namespace oscope {

class CrashLoopRecoveryHandler {
public:
    explicit CrashLoopRecoveryHandler(int maxRestartAttempts = 3);

    // Evaluates whether automatic restart is permitted or if max attempts exceeded
    bool shouldAttemptRestart(int currentAttemptCount, int maxAllowedAttempts) const;

    // Produces an escalation record when the crash loop threshold is exceeded
    RecoveryRecord handleExceededLimit(int incidentId, int attemptCount);

private:
    int defaultMaxAttempts_{3};
};

} // namespace oscope
