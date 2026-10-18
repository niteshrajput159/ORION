#include "monitoring/process_monitor.hpp"
#include "common/utils.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#if defined(__APPLE__)
#include <libproc.h>
#include <sys/sysctl.h>
#endif

namespace oscope {

ProcessMonitor::ProcessMonitor() {}

std::vector<int> ProcessMonitor::discoverActivePids() {
    std::vector<int> pids;

#if defined(__linux__)
    DIR* procDir = opendir("/proc");
    if (!procDir) return pids;

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            char* endPtr = nullptr;
            long pid = std::strtol(entry->d_name, &endPtr, 10);
            if (endPtr != entry->d_name && *endPtr == '\0' && pid > 0) {
                pids.push_back(static_cast<int>(pid));
            }
        }
    }
    closedir(procDir);
#elif defined(__APPLE__)
    int numberOfProcesses = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
    if (numberOfProcesses > 0) {
        std::vector<pid_t> pidList(numberOfProcesses);
        int bytesReturned = proc_listpids(PROC_ALL_PIDS, 0, pidList.data(), numberOfProcesses * sizeof(pid_t));
        int count = bytesReturned / sizeof(pid_t);
        for (int i = 0; i < count; ++i) {
            if (pidList[i] > 0) {
                pids.push_back(pidList[i]);
            }
        }
    }
#endif

    return pids;
}

std::string ProcessMonitor::readCmdLine(int pid) {
#if defined(__linux__)
    std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";

    std::string cmd;
    char ch;
    while (file.get(ch)) {
        if (ch == '\0') cmd += ' ';
        else cmd += ch;
    }
    return utils::trim(cmd);
#elif defined(__APPLE__)
    char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, pathBuffer, sizeof(pathBuffer)) > 0) {
        return std::string(pathBuffer);
    }
    return "";
#else
    return "";
#endif
}

int ProcessMonitor::countOpenFds(int pid) {
#if defined(__linux__)
    std::string fdDirPath = "/proc/" + std::to_string(pid) + "/fd";
    DIR* dir = opendir(fdDirPath.c_str());
    if (!dir) return 0;

    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] != '.') {
            count++;
        }
    }
    closedir(dir);
    return count;
#elif defined(__APPLE__)
    int bufferSize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, 0, 0);
    if (bufferSize > 0) {
        return bufferSize / PROC_PIDLISTFD_SIZE;
    }
    return 0;
#else
    return 0;
#endif
}

bool ProcessMonitor::parseProcIo(int pid, ProcessMetrics& metrics) {
#if defined(__linux__)
    std::string ioPath = "/proc/" + std::to_string(pid) + "/io";
    std::ifstream file(ioPath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("read_bytes:", 0) == 0) {
            std::istringstream ss(line.substr(11));
            ss >> metrics.diskReadBytes;
        } else if (line.rfind("write_bytes:", 0) == 0) {
            std::istringstream ss(line.substr(12));
            ss >> metrics.diskWriteBytes;
        }
    }
    return true;
#else
    (void)pid;
    (void)metrics;
    return false;
#endif
}

bool ProcessMonitor::parseProcStatus(int pid, ProcessInfo& info) {
#if defined(__linux__)
    std::string statusPath = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream file(statusPath);
    if (!file.is_open()) return false;

    std::string line;
    uint64_t vmrssKb = 0, vmsizeKb = 0;
    uint64_t vCtx = 0, nvCtx = 0;
    uid_t uid = 0;

    while (std::getline(file, line)) {
        if (line.rfind("Name:", 0) == 0) {
            if (info.identity.processName.empty()) {
                info.identity.processName = utils::trim(line.substr(5));
            }
        } else if (line.rfind("State:", 0) == 0) {
            std::string stateStr = utils::trim(line.substr(6));
            if (!stateStr.empty()) {
                info.currentMetrics.state = charToProcessState(stateStr[0]);
            }
        } else if (line.rfind("PPid:", 0) == 0) {
            info.ppid = std::stoi(utils::trim(line.substr(5)));
        } else if (line.rfind("Uid:", 0) == 0) {
            std::istringstream ss(line.substr(4));
            ss >> uid;
            struct passwd* pwd = getpwuid(uid);
            if (pwd) {
                info.owner = pwd->pw_name;
            }
        } else if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream ss(line.substr(6));
            ss >> vmrssKb;
        } else if (line.rfind("VmSize:", 0) == 0) {
            std::istringstream ss(line.substr(7));
            ss >> vmsizeKb;
        } else if (line.rfind("Threads:", 0) == 0) {
            info.currentMetrics.threadCount = std::stoi(utils::trim(line.substr(8)));
        } else if (line.rfind("voluntary_ctxt_switches:", 0) == 0) {
            std::istringstream ss(line.substr(24));
            ss >> vCtx;
        } else if (line.rfind("nonvoluntary_ctxt_switches:", 0) == 0) {
            std::istringstream ss(line.substr(27));
            ss >> nvCtx;
        }
    }

    info.currentMetrics.memoryRssMb = vmrssKb / 1024.0;
    info.currentMetrics.virtualMemoryMb = vmsizeKb / 1024.0;
    info.currentMetrics.contextSwitches = vCtx + nvCtx;
    return true;
#else
    (void)pid;
    (void)info;
    return false;
#endif
}

bool ProcessMonitor::parseProcStat(int pid, ProcessInfo& info) {
#if defined(__linux__)
    std::string statPath = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(statPath);
    if (!file.is_open()) return false;

    std::string content;
    if (!std::getline(file, content)) return false;

    size_t firstParen = content.find('(');
    size_t lastParen = content.rfind(')');
    if (firstParen == std::string::npos || lastParen == std::string::npos || lastParen <= firstParen) {
        return false;
    }

    if (info.identity.processName.empty()) {
        info.identity.processName = content.substr(firstParen + 1, lastParen - firstParen - 1);
    }

    std::string rest = content.substr(lastParen + 2);
    std::istringstream ss(rest);
    std::vector<std::string> tokens;
    std::string tok;
    while (ss >> tok) {
        tokens.push_back(tok);
    }

    // Relative to lastParen + 2:
    // tokens[0]: state (field 3)
    // tokens[1]: ppid (field 4)
    // tokens[7]: minflt (field 10)
    // tokens[9]: majflt (field 12)
    // tokens[15]: priority (field 18)
    // tokens[16]: nice (field 19)
    // tokens[19]: starttime (field 22)
    if (tokens.size() >= 20) {
        try {
            if (!tokens[0].empty()) {
                info.currentMetrics.state = charToProcessState(tokens[0][0]);
            }
            info.ppid = std::stoi(tokens[1]);
            uint64_t minflt = std::stoull(tokens[7]);
            uint64_t majflt = std::stoull(tokens[9]);
            info.currentMetrics.pageFaults = minflt + majflt;
            info.currentMetrics.priority = std::stoi(tokens[16]); // nice value
            info.identity.startTimeJiffies = std::stoull(tokens[19]);
        } catch (...) {
            // Ignore token parse error
        }
    }
    return true;
#elif defined(__APPLE__)
    struct proc_taskinfo pti;
    int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));
    if (ret == sizeof(pti)) {
        info.currentMetrics.memoryRssMb = pti.pti_resident_size / (1024.0 * 1024.0);
        info.currentMetrics.virtualMemoryMb = pti.pti_virtual_size / (1024.0 * 1024.0);
        info.currentMetrics.threadCount = pti.pti_threadnum;
        info.currentMetrics.contextSwitches = pti.pti_csw;
        info.currentMetrics.pageFaults = pti.pti_faults;
        info.currentMetrics.state = ProcessState::RUNNING;
        info.identity.startTimeJiffies = 1000;
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool ProcessMonitor::readProcessDetails(int pid, ProcessInfo& outInfo) {
    if (!utils::isPidAlive(pid)) {
        return false;
    }

    outInfo.identity.pid = pid;
    outInfo.currentMetrics.timestamp = std::chrono::system_clock::now();

#if defined(__linux__)
    parseProcStat(pid, outInfo);
    parseProcStatus(pid, outInfo);
    parseProcIo(pid, outInfo.currentMetrics);
    outInfo.currentMetrics.openFileDescriptors = countOpenFds(pid);
    outInfo.identity.commandLine = readCmdLine(pid);
#elif defined(__APPLE__)
    char pathBuf[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, pathBuf, sizeof(pathBuf)) > 0) {
        outInfo.identity.commandLine = pathBuf;
        std::string p(pathBuf);
        size_t slash = p.rfind('/');
        outInfo.identity.processName = (slash != std::string::npos) ? p.substr(slash + 1) : p;
    }
    parseProcStat(pid, outInfo);
    outInfo.currentMetrics.openFileDescriptors = countOpenFds(pid);
#endif

    // Compute real CPU percentage
    outInfo.currentMetrics.cpuUsagePercent = cpuCalculator_.calculateProcessCpu(pid);

    // Set process status enum
    switch (outInfo.currentMetrics.state) {
        case ProcessState::RUNNING: outInfo.status = ProcessStatus::RUNNING; break;
        case ProcessState::SLEEPING:
        case ProcessState::DISK_SLEEP: outInfo.status = ProcessStatus::SLEEPING; break;
        case ProcessState::STOPPED: outInfo.status = ProcessStatus::STOPPED; break;
        case ProcessState::ZOMBIE: outInfo.status = ProcessStatus::ZOMBIE; break;
        default: outInfo.status = ProcessStatus::RUNNING; break;
    }

    return true;
}

std::vector<ProcessInfo> ProcessMonitor::scanAllProcesses() {
    std::vector<int> pids = discoverActivePids();
    std::vector<ProcessInfo> results;
    results.reserve(pids.size());

    for (int pid : pids) {
        ProcessInfo info;
        if (readProcessDetails(pid, info)) {
            results.push_back(info);
        }
    }
    return results;
}

bool ProcessMonitor::refreshProcess(ProcessInfo& proc) {
    ProcessInfo updated;
    if (readProcessDetails(proc.identity.pid, updated)) {
        // Retain identity invariants
        if (proc.identity.startTimeJiffies > 0 &&
            updated.identity.startTimeJiffies > 0 &&
            proc.identity.startTimeJiffies != updated.identity.startTimeJiffies) {
            // PID reuse detected!
            return false;
        }

        proc.currentMetrics = updated.currentMetrics;
        proc.status = updated.status;
        proc.ppid = updated.ppid;
        proc.recentHistory.push_back(updated.currentMetrics);
        if (proc.recentHistory.size() > 60) {
            proc.recentHistory.erase(proc.recentHistory.begin());
        }
        return true;
    }
    return false;
}

} // namespace oscope
