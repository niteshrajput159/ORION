#pragma once

#include "communication/protocol.hpp"
#include "monitoring/process_monitor.hpp"
#include "process/process_controller.hpp"
#include "process/process_registry.hpp"
#include "verification/verification_engine.hpp"
#include "deadlock/deadlock_detector.hpp"

namespace oscope {

class CommandHandler {
public:
    CommandHandler(ProcessMonitor& monitor,
                   ProcessController& controller,
                   VerificationEngine& verification,
                   DeadlockDetector& deadlockDetector);

    protocol::CommandResponse handleCommand(const protocol::CommandRequest& request);

private:
    ProcessMonitor& monitor_;
    ProcessController& controller_;
    VerificationEngine& verification_;
    DeadlockDetector& deadlockDetector_;
};

} // namespace oscope
