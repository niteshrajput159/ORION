#pragma once

#include "common/types.hpp"
#include "process/process_controller.hpp"
#include "verification/verification_engine.hpp"

namespace oscope {

class CrashRecoveryHandler {
public:
    CrashRecoveryHandler(ProcessController& controller, VerificationEngine& verification);

    // Executes bounded crash recovery: restarts process and verifies it is running
    RecoveryRecord handleCrash(int oldPid, const std::string& commandLine, int attemptNumber, int& outNewPid);

private:
    ProcessController& controller_;
    VerificationEngine& verification_;
};

} // namespace oscope
