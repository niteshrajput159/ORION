#pragma once

#include "common/types.hpp"
#include <string>
#include <vector>

namespace oscope {

class ProcessController {
public:
    ProcessController() = default;

    // Gracefully terminates a process using SIGTERM
    ControlResult terminateSigterm(int pid);

    // Forcefully terminates a process using SIGKILL
    ControlResult terminateSigkill(int pid);

    // Suspends process execution using SIGSTOP
    ControlResult suspendProcess(int pid);

    // Resumes suspended process execution using SIGCONT
    ControlResult resumeProcess(int pid);

    // Modifies process priority via setpriority (nice value: -20 to 19)
    ControlResult changePriority(int pid, int niceValue);

    // Restarts process using stored command line arguments
    ControlResult restartProcess(int pid, const std::string& commandLine, int& outNewPid);

    // Executes a generic recovery action enum
    ControlResult executeAction(RecoveryAction action, int pid, const std::string& commandLine, int& outNewPid);

private:
    ControlResult sendSignal(int pid, int signalNumber, const std::string& actionName);
};

} // namespace oscope
