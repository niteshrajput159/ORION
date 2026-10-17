#include "communication/protocol.hpp"
#include "monitoring/process_monitor.hpp"
#include "process/process_controller.hpp"
#include "process/process_registry.hpp"
#include "verification/verification_engine.hpp"
#include "deadlock/deadlock_detector.hpp"
#include "communication/command_handler.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

int sendViaIpcSocket(const std::string& socketPath, const std::string& commandLine) {
    int sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockFd < 0) return -1;

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockFd);
        return -1;
    }

    std::string req = commandLine + "\n";
    ssize_t written = write(sockFd, req.c_str(), req.length());
    (void)written;

    char buffer[8192];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = read(sockFd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        std::cout << buffer;
    }

    close(sockFd);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: oscope_ctl <COMMAND> [args...]" << std::endl;
        std::cerr << "Commands:" << std::endl;
        std::cerr << "  GET_PROCESS_INFO <pid>" << std::endl;
        std::cerr << "  TERMINATE_PROCESS <pid> [SIGTERM|SIGKILL]" << std::endl;
        std::cerr << "  SUSPEND_PROCESS <pid>" << std::endl;
        std::cerr << "  RESUME_PROCESS <pid>" << std::endl;
        std::cerr << "  CHANGE_PRIORITY <pid> <nice>" << std::endl;
        std::cerr << "  RESTART_PROCESS <pid> [cmdLine]" << std::endl;
        std::cerr << "  VERIFY_PROCESS <pid>" << std::endl;
        std::cerr << "  CHECK_DEADLOCK [stateFilePath]" << std::endl;
        std::cerr << "  REGISTER_PROCESS <pid> <name> [cmdLine]" << std::endl;
        std::cerr << "  UNREGISTER_PROCESS <pid>" << std::endl;
        std::cerr << "  GET_SYSTEM_HEALTH" << std::endl;
        std::cerr << "  GET_ALL_PROCESSES" << std::endl;
        return 1;
    }

    std::string commandLine;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) commandLine += " ";
        commandLine += argv[i];
    }

    // Attempt 1: Try sending via IPC socket to active running daemon
    std::string socketPath = "/tmp/oscope_core.sock";
    if (sendViaIpcSocket(socketPath, commandLine) == 0) {
        return 0;
    }

    // Attempt 2: If daemon is not running on socket, execute command directly in standalone mode
    oscope::ProcessMonitor monitor;
    oscope::ProcessController controller;
    oscope::VerificationEngine verification;
    oscope::DeadlockDetector deadlockDetector;
    oscope::CommandHandler handler(monitor, controller, verification, deadlockDetector);

    oscope::protocol::CommandRequest req = oscope::protocol::parseRequestLine(commandLine);
    oscope::protocol::CommandResponse resp = handler.handleCommand(req);

    std::cout << resp.toJsonString() << std::endl;
    return resp.success ? 0 : 1;
}
