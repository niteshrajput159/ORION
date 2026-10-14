#include <iostream>
#include <vector>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <string>

std::atomic<bool> g_running{true};

void sigHandler(int sig) {
    (void)sig;
    g_running = false;
}

int main(int argc, char* argv[]) {
    int targetMb = 300;
    int rateMbPerSec = 50;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--target-mb" && i + 1 < argc) {
            targetMb = std::stoi(argv[++i]);
        } else if (arg == "--rate-mb" && i + 1 < argc) {
            rateMbPerSec = std::stoi(argv[++i]);
        }
    }

    std::signal(SIGTERM, sigHandler);
    std::signal(SIGINT, sigHandler);

    std::cout << "[memory_hog] Starting real memory allocation workload (PID " << getpid() << ")" << std::endl;
    std::cout << "[memory_hog] Target allocation: " << targetMb << " MB RSS at rate " << rateMbPerSec << " MB/s..." << std::endl;

    std::vector<char*> allocatedBlocks;
    const size_t blockSize = 10 * 1024 * 1024; // 10 MB per chunk
    size_t totalAllocatedBytes = 0;
    size_t targetBytes = static_cast<size_t>(targetMb) * 1024 * 1024;

    while (g_running && totalAllocatedBytes < targetBytes) {
        char* block = new (std::nothrow) char[blockSize];
        if (!block) {
            std::cerr << "[memory_hog] Out of memory allocation failure!" << std::endl;
            break;
        }

        // Crucial: Touch every 4KB page so that Linux assigns real physical RAM (RSS)
        for (size_t offset = 0; offset < blockSize; offset += 4096) {
            block[offset] = static_cast<char>((offset + totalAllocatedBytes) % 256);
        }

        allocatedBlocks.push_back(block);
        totalAllocatedBytes += blockSize;

        std::cout << "[memory_hog] Allocated and touched: " << (totalAllocatedBytes / (1024 * 1024))
                  << " MB physical RSS" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * blockSize / (static_cast<size_t>(rateMbPerSec) * 1024 * 1024)));
    }

    std::cout << "[memory_hog] Reached target memory. Holding allocated pages in memory..." << std::endl;
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[memory_hog] Cleaning up memory allocations..." << std::endl;
    for (char* blk : allocatedBlocks) {
        delete[] blk;
    }

    std::cout << "[memory_hog] Exited cleanly." << std::endl;
    return 0;
}
