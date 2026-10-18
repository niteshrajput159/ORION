#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <chrono>
#include <csignal>
#include <unistd.h>
#include <atomic>

std::mutex g_mutexA;
std::mutex g_mutexB;
std::mutex g_fileMutex;

std::atomic<bool> g_running{true};
std::string g_stateFile = "/tmp/oscope_deadlock.state";

void writeState(const std::string& action, int threadId, const std::string& resource) {
    std::lock_guard<std::mutex> lock(g_fileMutex);
    std::ofstream out(g_stateFile, std::ios::app);
    if (out.is_open()) {
        out << action << " " << threadId << " " << resource << "\n";
        out.flush();
    }
}

void sigHandler(int sig) {
    (void)sig;
    g_running = false;
    unlink(g_stateFile.c_str());
    _exit(0);
}

void thread1Func() {
    int tid = 1;
    std::cout << "[deadlock_test] Thread 1 attempting to acquire Mutex A..." << std::endl;
    g_mutexA.lock();
    writeState("HOLD", tid, "Mutex_A");
    std::cout << "[deadlock_test] Thread 1 acquired Mutex A. Sleeping 200ms..." << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "[deadlock_test] Thread 1 waiting for Mutex B (held by Thread 2)..." << std::endl;
    writeState("WAIT", tid, "Mutex_B");
    g_mutexB.lock(); // Blocks here in real deadlock!

    std::cout << "[deadlock_test] Thread 1 acquired Mutex B (Deadlock resolved)." << std::endl;
    g_mutexB.unlock();
    g_mutexA.unlock();
}

void thread2Func() {
    int tid = 2;
    std::cout << "[deadlock_test] Thread 2 attempting to acquire Mutex B..." << std::endl;
    g_mutexB.lock();
    writeState("HOLD", tid, "Mutex_B");
    std::cout << "[deadlock_test] Thread 2 acquired Mutex B. Sleeping 200ms..." << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "[deadlock_test] Thread 2 waiting for Mutex A (held by Thread 1)..." << std::endl;
    writeState("WAIT", tid, "Mutex_A");
    g_mutexA.lock(); // Blocks here in real deadlock!

    std::cout << "[deadlock_test] Thread 2 acquired Mutex A (Deadlock resolved)." << std::endl;
    g_mutexA.unlock();
    g_mutexB.unlock();
}

int main() {
    std::signal(SIGTERM, sigHandler);
    std::signal(SIGINT, sigHandler);

    // Truncate existing state file and record workload PID
    {
        std::ofstream out(g_stateFile, std::ios::trunc);
        out << "# OScope Deadlock State Instrumentation\n";
        out << "PID " << getpid() << " deadlock_test\n";
    }

    std::cout << "[deadlock_test] Deadlock workload started (PID " << getpid() << ")" << std::endl;
    std::cout << "[deadlock_test] Creating 2 threads with circular lock dependencies (A -> B and B -> A)..." << std::endl;

    std::thread t1(thread1Func);
    std::thread t2(thread2Func);

    t1.join();
    t2.join();

    unlink(g_stateFile.c_str());
    std::cout << "[deadlock_test] Workload finished." << std::endl;
    return 0;
}
