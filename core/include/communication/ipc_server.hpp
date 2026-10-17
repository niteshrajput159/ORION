#pragma once

#include "communication/command_handler.hpp"
#include <string>
#include <thread>
#include <atomic>

namespace oscope {

class IpcServer {
public:
    IpcServer(const std::string& socketPath, CommandHandler& handler);
    ~IpcServer();

    // Starts listening in a dedicated background thread
    bool start();

    // Stops the IPC server and removes the socket file
    void stop();

    bool isRunning() const { return running_; }

private:
    void runServerLoop();
    void handleClient(int clientFd);

    std::string socketPath_;
    CommandHandler& handler_;
    int serverFd_{-1};
    std::atomic<bool> running_{false};
    std::thread serverThread_;
};

} // namespace oscope
