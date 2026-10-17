#include "process/process_controller.hpp"
#include "common/utils.hpp"
#include <csignal>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <iostream>

namespace oscope {

ControlResult ProcessController::sendSignal(int pid, int signalNumber, const std::string& actionName) {
    ControlResult result;
    result.pid = pid;
    result.action = actionName;
    result.timestamp = std::chrono::system_clock::now();
    uint64_t startMs = utils::getMonotonicTimeMs();

    if (pid <= 0) {
        result.success = false;
        result.errorMessage = "Invalid PID " + std::to_string(pid);
        result.durationMs = 0.0;
        return result;
    }

    if (!utils::isPidAlive(pid)) {
        result.success = false;
        result.errorMessage = "Process " + std::to_string(pid) + " is not running (ESRCH)";
        result.durationMs = 0.0;
        return result;
    }

    if (kill(pid, signalNumber) == 0) {
        result.success = true;
    } else {
        result.success = false;
        result.errorMessage = std::strerror(errno);
    }

    result.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
    return result;
}

ControlResult ProcessController::terminateSigterm(int pid) {
    return sendSignal(pid, SIGTERM, "SIGTERM");
}

ControlResult ProcessController::terminateSigkill(int pid) {
    return sendSignal(pid, SIGKILL, "SIGKILL");
}

ControlResult ProcessController::suspendProcess(int pid) {
    return sendSignal(pid, SIGSTOP, "SIGSTOP");
}

ControlResult ProcessController::resumeProcess(int pid) {
    return sendSignal(pid, SIGCONT, "SIGCONT");
}

ControlResult ProcessController::changePriority(int pid, int niceValue) {
    ControlResult result;
    result.pid = pid;
    result.action = "CHANGE_PRIORITY";
    result.timestamp = std::chrono::system_clock::now();
    uint64_t startMs = utils::getMonotonicTimeMs();

    if (pid <= 0) {
        result.success = false;
        result.errorMessage = "Invalid PID " + std::to_string(pid);
        return result;
    }

    // Clamp nice value between -20 and 19
    int clampedNice = std::max(-20, std::min(19, niceValue));

    // Clear errno before call since setpriority can return -1 on success
    errno = 0;
    int ret = setpriority(PRIO_PROCESS, pid, clampedNice);
    if (ret == 0 || errno == 0) {
        result.success = true;
    } else {
        result.success = false;
        result.errorMessage = std::strerror(errno);
    }

    result.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
    return result;
}

ControlResult ProcessController::restartProcess(int pid, const std::string& commandLine, int& outNewPid) {
    ControlResult result;
    result.pid = pid;
    result.action = "RESTART";
    result.timestamp = std::chrono::system_clock::now();
    outNewPid = 0;
    uint64_t startMs = utils::getMonotonicTimeMs();

    if (commandLine.empty()) {
        result.success = false;
        result.errorMessage = "Cannot restart: empty command line specification";
        return result;
    }

    // Gracefully terminate existing process if still running
    if (pid > 0 && utils::isPidAlive(pid)) {
        kill(pid, SIGTERM);
        utils::sleepMs(100);
        if (utils::isPidAlive(pid)) {
            kill(pid, SIGKILL);
            utils::sleepMs(50);
        }
    }

    // Parse command line into arguments vector
    std::vector<std::string> argTokens = utils::split(commandLine, ' ');
    if (argTokens.empty()) {
        result.success = false;
        result.errorMessage = "Failed to parse command arguments";
        return result;
    }

    std::vector<char*> cArgs;
    cArgs.reserve(argTokens.size() + 1);
    for (auto& token : argTokens) {
        cArgs.push_back(const_cast<char*>(token.c_str()));
    }
    cArgs.push_back(nullptr);

    pid_t newPid = fork();
    if (newPid < 0) {
        result.success = false;
        result.errorMessage = std::string("fork() failed: ") + std::strerror(errno);
        result.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
        return result;
    }

    if (newPid == 0) {
        // Child process
        // Redirect stdout and stderr if desired or run in new session
        setsid();
        execvp(cArgs[0], cArgs.data());
        // If execvp returns, execution failed
        std::cerr << "execvp failed for " << cArgs[0] << ": " << std::strerror(errno) << std::endl;
        _exit(127);
    }

    // Parent process
    outNewPid = static_cast<int>(newPid);
    result.success = true;
    result.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
    return result;
}

ControlResult ProcessController::executeAction(RecoveryAction action, int pid, const std::string& commandLine, int& outNewPid) {
    outNewPid = 0;
    switch (action) {
        case RecoveryAction::LOWER_PRIORITY:
            // Lower CPU priority by increasing nice value (+10)
            return changePriority(pid, 10);

        case RecoveryAction::SUSPEND:
            return suspendProcess(pid);

        case RecoveryAction::RESUME:
            return resumeProcess(pid);

        case RecoveryAction::TERMINATE_SIGTERM:
            return terminateSigterm(pid);

        case RecoveryAction::TERMINATE_SIGKILL:
            return terminateSigkill(pid);

        case RecoveryAction::RESTART:
            return restartProcess(pid, commandLine, outNewPid);

        case RecoveryAction::ALERT_ONLY: {
            ControlResult res;
            res.success = true;
            res.pid = pid;
            res.action = "ALERT_ONLY";
            res.timestamp = std::chrono::system_clock::now();
            return res;
        }

        default: {
            ControlResult res;
            res.success = false;
            res.pid = pid;
            res.action = "UNKNOWN";
            res.errorMessage = "Unsupported recovery action";
            return res;
        }
    }
}

} // namespace oscope
