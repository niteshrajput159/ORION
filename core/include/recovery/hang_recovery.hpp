#pragma once

#include "common/types.hpp"
#include "process/process_controller.hpp"
#include "verification/verification_engine.hpp"

namespace oscope {

class HangRecoveryHandler {
public:
    HangRecoveryHandler(ProcessController& controller, VerificationEngine& verification);

    // Executes staged hung process recovery: SIGTERM -> wait -> verify -> SIGKILL escalation if still hung
    RecoveryRecord handleHungProcess(int pid, int sigtermTimeoutSeconds);

private:
    ProcessController& controller_;
    VerificationEngine& verification_;
};

} // namespace oscope
