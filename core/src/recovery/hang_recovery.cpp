#include "recovery/hang_recovery.hpp"
#include "common/utils.hpp"

namespace oscope {

HangRecoveryHandler::HangRecoveryHandler(ProcessController& controller, VerificationEngine& verification)
    : controller_(controller), verification_(verification) {}

RecoveryRecord HangRecoveryHandler::handleHungProcess(int pid, int sigtermTimeoutSeconds) {
    RecoveryRecord rec;
    rec.action = RecoveryAction::TERMINATE_SIGTERM;
    rec.startedAtIso = utils::getCurrentIsoTime();
    uint64_t startMs = utils::getMonotonicTimeMs();

    // Step 1: Send graceful SIGTERM
    ControlResult termResult = controller_.terminateSigterm(pid);
    if (!termResult.success) {
        rec.success = false;
        rec.result = RecoveryResult::VERIFIED_FAILURE;
        rec.verificationDetails = "Failed to send initial SIGTERM: " + termResult.errorMessage;
        rec.completedAtIso = utils::getCurrentIsoTime();
        rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
        return rec;
    }

    // Step 2: Verify termination with timeout
    bool needsSigkill = false;
    std::string verifyDetails;
    bool termSuccess = verification_.verifyTermination(pid, sigtermTimeoutSeconds, needsSigkill, verifyDetails);

    if (termSuccess) {
        rec.success = true;
        rec.result = RecoveryResult::VERIFIED_SUCCESS;
        rec.verificationDetails = "Graceful termination verified: " + verifyDetails;
        rec.completedAtIso = utils::getCurrentIsoTime();
        rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
        return rec;
    }

    // Step 3: Staged escalation to SIGKILL if still hung
    if (needsSigkill) {
        rec.action = RecoveryAction::TERMINATE_SIGKILL;
        ControlResult killResult = controller_.terminateSigkill(pid);
        if (!killResult.success) {
            rec.success = false;
            rec.result = RecoveryResult::VERIFIED_FAILURE;
            rec.verificationDetails = "SIGTERM timed out; subsequent SIGKILL failed: " + killResult.errorMessage;
            rec.completedAtIso = utils::getCurrentIsoTime();
            rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
            return rec;
        }

        // Verify SIGKILL result
        bool dummyNeedsKill = false;
        std::string killVerifyDetails;
        bool killSuccess = verification_.verifyTermination(pid, 2, dummyNeedsKill, killVerifyDetails);

        rec.success = killSuccess;
        rec.result = killSuccess ? RecoveryResult::ESCALATED_SIGKILL : RecoveryResult::VERIFIED_FAILURE;
        rec.verificationDetails = "Escalated to SIGKILL: " + killVerifyDetails;
        rec.completedAtIso = utils::getCurrentIsoTime();
        rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
        return rec;
    }

    rec.success = false;
    rec.result = RecoveryResult::VERIFIED_FAILURE;
    rec.verificationDetails = verifyDetails;
    rec.completedAtIso = utils::getCurrentIsoTime();
    rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
    return rec;
}

} // namespace oscope
