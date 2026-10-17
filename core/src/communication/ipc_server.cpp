#include "communication/ipc_server.hpp"
#include "common/logger.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <vector>

namespace oscope {

IpcServer::IpcServer(const std::string& socketPath, CommandHandler& handler)
    : socketPath_(socketPath), handler_(handler) {}

IpcServer::~IpcServer() {
    stop();
}

bool IpcServer::start() {
    if (running_) return true;

    // Unlink old socket file if present
    unlink(socketPath_.c_str());

    serverFd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        LOG_ERROR("IpcServer", std::string("Failed to create UNIX domain socket: ") + std::strerror(errno));
        return false;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath_.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(serverFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("IpcServer", std::string("Failed to bind socket: ") + std::strerror(errno));
        close(serverFd_);
        serverFd_ = -1;
        return false;
    }

    if (listen(serverFd_, 16) < 0) {
        LOG_ERROR("IpcServer", std::string("Failed to listen on socket: ") + std::strerror(errno));
        close(serverFd_);
        serverFd_ = -1;
        return false;
    }

    running_ = true;
    serverThread_ = std::thread(&IpcServer::runServerLoop, this);
    LOG_INFO("IpcServer", "UNIX Domain Socket IPC server started on " + socketPath_);
    return true;
}

void IpcServer::stop() {
    if (!running_) return;
    running_ = false;

    if (serverFd_ >= 0) {
        shutdown(serverFd_, SHUT_RDWR);
        close(serverFd_);
        serverFd_ = -1;
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    unlink(socketPath_.c_str());
    LOG_INFO("IpcServer", "UNIX Domain Socket IPC server stopped.");
}

void IpcServer::runServerLoop() {
    while (running_) {
        struct sockaddr_un clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientFd < 0) {
            if (!running_) break;
            continue;
        }

        handleClient(clientFd);
    }
}

void IpcServer::handleClient(int clientFd) {
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        std::string requestLine(buffer);
        protocol::CommandRequest req = protocol::parseRequestLine(requestLine);
        protocol::CommandResponse resp = handler_.handleCommand(req);
        std::string respJson = resp.toJsonString() + "\n";
        ssize_t written = write(clientFd, respJson.c_str(), respJson.length());
        (void)written;
    }

    close(clientFd);
}

} // namespace oscope
