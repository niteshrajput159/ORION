#include "recovery/crash_loop_recovery.hpp"
#include "common/utils.hpp"

namespace oscope {

CrashLoopRecoveryHandler::CrashLoopRecoveryHandler(int maxRestartAttempts)
    : defaultMaxAttempts_(maxRestartAttempts) {}

bool CrashLoopRecoveryHandler::shouldAttemptRestart(int currentAttemptCount, int maxAllowedAttempts) const {
    int maxLimit = (maxAllowedAttempts > 0) ? maxAllowedAttempts : defaultMaxAttempts_;
    return currentAttemptCount < maxLimit;
}

RecoveryRecord CrashLoopRecoveryHandler::handleExceededLimit(int incidentId, int attemptCount) {
    RecoveryRecord rec;
    rec.incidentId = incidentId;
    rec.attemptNumber = attemptCount + 1;
    rec.action = RecoveryAction::ALERT_ONLY;
    rec.startedAtIso = utils::getCurrentIsoTime();
    rec.completedAtIso = rec.startedAtIso;
    rec.result = RecoveryResult::MAX_ATTEMPTS_EXCEEDED;
    rec.success = false;
    rec.verificationDetails = "CRASH_LOOP BOUND REACHED: " + std::to_string(attemptCount) +
                             " restart attempts failed. Automated recovery halted; administrative escalation triggered.";
    rec.durationMs = 0.0;
    return rec;
}

} // namespace oscope
