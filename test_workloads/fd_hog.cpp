#include <iostream>
#include <vector>
#include <unistd.h>
#include <csignal>
#include <chrono>
#include <thread>
#include <atomic>

std::atomic<bool> g_running{true};

void sigHandler(int sig) {
    (void)sig;
    g_running = false;
}

int main(int argc, char* argv[]) {
    int targetFds = 600;
    if (argc > 1) {
        targetFds = std::stoi(argv[1]);
    }

    std::signal(SIGTERM, sigHandler);
    std::signal(SIGINT, sigHandler);

    std::cout << "[fd_hog] File Descriptor Hog workload started (PID " << getpid() << ")" << std::endl;
    std::cout << "[fd_hog] Opening " << targetFds << " file descriptors using pipes..." << std::endl;

    std::vector<int> openedFds;
    openedFds.reserve(targetFds);

    while (openedFds.size() < static_cast<size_t>(targetFds) && g_running) {
        int pipeFds[2];
        if (pipe(pipeFds) == 0) {
            openedFds.push_back(pipeFds[0]);
            openedFds.push_back(pipeFds[1]);
        } else {
            std::cerr << "[fd_hog] pipe() failed (system limit reached at " << openedFds.size() << " FDs)" << std::endl;
            break;
        }
    }

    std::cout << "[fd_hog] Successfully opened " << openedFds.size() << " file descriptors. Holding open..." << std::endl;

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[fd_hog] Closing opened file descriptors..." << std::endl;
    for (int fd : openedFds) {
        close(fd);
    }

    std::cout << "[fd_hog] Cleanly exited." << std::endl;
    return 0;
}
