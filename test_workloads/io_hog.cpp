#include <iostream>
#include <vector>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <atomic>
#include <cstring>

std::atomic<bool> g_running{true};

void sigHandler(int sig) {
    (void)sig;
    g_running = false;
}

int main(int argc, char* argv[]) {
    int durationSec = 30;
    if (argc > 1) {
        durationSec = std::stoi(argv[1]);
    }

    std::signal(SIGTERM, sigHandler);
    std::signal(SIGINT, sigHandler);

    std::string testFilePath = "/tmp/oscope_io_test.dat";
    std::cout << "[io_hog] I/O Hog workload started (PID " << getpid() << ")" << std::endl;
    std::cout << "[io_hog] Generating heavy synchronized disk writes to " << testFilePath << "..." << std::endl;

    int fd = open(testFilePath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        std::cerr << "[io_hog] Failed to open temp file: " << std::strerror(errno) << std::endl;
        return 1;
    }

    const size_t chunkSize = 1024 * 1024; // 1 MB
    std::vector<char> buffer(chunkSize, 'A');

    auto start = std::chrono::steady_clock::now();
    size_t totalWrittenMb = 0;

    while (g_running) {
        ssize_t w = write(fd, buffer.data(), buffer.size());
        if (w > 0) {
            fsync(fd); // Force disk sync to register in kernel I/O counters
            totalWrittenMb += (w / (1024 * 1024));
        }

        // Seek back to prevent filling up the entire disk
        if (totalWrittenMb % 50 == 0) {
            lseek(fd, 0, SEEK_SET);
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
        if (elapsed >= durationSec) {
            break;
        }

        usleep(10000); // 10ms
    }

    close(fd);
    unlink(testFilePath.c_str());

    std::cout << "[io_hog] Total I/O written: " << totalWrittenMb << " MB. Workload completed." << std::endl;
    return 0;
}
