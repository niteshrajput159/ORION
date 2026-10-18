#include "recovery/crash_recovery.hpp"
#include "common/utils.hpp"

namespace oscope {

CrashRecoveryHandler::CrashRecoveryHandler(ProcessController& controller, VerificationEngine& verification)
    : controller_(controller), verification_(verification) {}

RecoveryRecord CrashRecoveryHandler::handleCrash(int oldPid, const std::string& commandLine, int attemptNumber, int& outNewPid) {
    RecoveryRecord rec;
    rec.attemptNumber = attemptNumber;
    rec.action = RecoveryAction::RESTART;
    rec.startedAtIso = utils::getCurrentIsoTime();
    uint64_t startMs = utils::getMonotonicTimeMs();

    ControlResult ctrlRes = controller_.restartProcess(oldPid, commandLine, outNewPid);
    if (!ctrlRes.success) {
        rec.success = false;
        rec.result = RecoveryResult::VERIFIED_FAILURE;
        rec.verificationDetails = "Restart execution failed: " + ctrlRes.errorMessage;
        rec.completedAtIso = utils::getCurrentIsoTime();
        rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
        return rec;
    }

    // Verify the restarted process is running
    std::string verifyDetails;
    bool verified = verification_.verifyRestart(outNewPid, 100, verifyDetails);

    rec.success = verified;
    rec.result = verified ? RecoveryResult::VERIFIED_SUCCESS : RecoveryResult::VERIFIED_FAILURE;
    rec.verificationDetails = verifyDetails;
    rec.completedAtIso = utils::getCurrentIsoTime();
    rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);

    return rec;
}

} // namespace oscope
