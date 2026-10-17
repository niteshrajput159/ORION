#include "communication/command_handler.hpp"
#include "common/utils.hpp"
#include <sstream>
#include <iomanip>

namespace oscope {

CommandHandler::CommandHandler(ProcessMonitor& monitor,
                               ProcessController& controller,
                               VerificationEngine& verification,
                               DeadlockDetector& deadlockDetector)
    : monitor_(monitor),
      controller_(controller),
      verification_(verification),
      deadlockDetector_(deadlockDetector) {}

protocol::CommandResponse CommandHandler::handleCommand(const protocol::CommandRequest& req) {
    protocol::CommandResponse resp;

    if (req.command == protocol::CMD_GET_PROCESS_INFO) {
        if (req.args.empty()) {
            resp.success = false;
            resp.errorMessage = "Missing PID argument";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        ProcessInfo info;
        if (monitor_.readProcessDetails(pid, info)) {
            std::ostringstream ss;
            ss << "{\"pid\":" << info.identity.pid
               << ",\"name\":\"" << utils::escapeJson(info.identity.processName) << "\""
               << ",\"ppid\":" << info.ppid
               << ",\"owner\":\"" << utils::escapeJson(info.owner) << "\""
               << ",\"state\":\"" << processStateToString(info.currentMetrics.state) << "\""
               << ",\"status\":\"" << (info.status == ProcessStatus::RUNNING ? "RUNNING" : "SLEEPING") << "\""
               << ",\"cpu_usage\":" << info.currentMetrics.cpuUsagePercent
               << ",\"memory_rss_mb\":" << info.currentMetrics.memoryRssMb
               << ",\"threads\":" << info.currentMetrics.threadCount
               << ",\"priority\":" << info.currentMetrics.priority
               << ",\"context_switches\":" << info.currentMetrics.contextSwitches
               << ",\"page_faults\":" << info.currentMetrics.pageFaults
               << ",\"open_fds\":" << info.currentMetrics.openFileDescriptors
               << ",\"is_registered\":" << (ProcessRegistry::instance().isRegistered(pid) ? "true" : "false")
               << "}";
            resp.success = true;
            resp.dataJson = ss.str();
        } else {
            resp.success = false;
            resp.errorMessage = "Process " + std::to_string(pid) + " not found or dead";
        }
        return resp;
    }

    if (req.command == protocol::CMD_TERMINATE_PROCESS) {
        if (req.args.empty()) {
            resp.success = false;
            resp.errorMessage = "Missing PID argument";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        std::string mode = (req.args.size() > 1) ? req.args[1] : "SIGTERM";

        ControlResult res = (mode == "SIGKILL") ? controller_.terminateSigkill(pid) : controller_.terminateSigterm(pid);
        bool needsKill = false;
        std::string verifyDetails;
        bool verified = verification_.verifyTermination(pid, (mode == "SIGKILL" ? 2 : 5), needsKill, verifyDetails);

        if (!verified && needsKill && mode != "SIGKILL") {
            // Escalate to SIGKILL
            controller_.terminateSigkill(pid);
            verified = verification_.verifyTermination(pid, 2, needsKill, verifyDetails);
            mode = "ESCALATED_SIGKILL";
        }

        std::ostringstream ss;
        ss << "{\"pid\":" << pid
           << ",\"mode\":\"" << mode << "\""
           << ",\"success\":" << (verified ? "true" : "false")
           << ",\"duration_ms\":" << res.durationMs
           << ",\"details\":\"" << utils::escapeJson(verifyDetails) << "\"}";
        resp.success = verified;
        resp.dataJson = ss.str();
        if (!verified) resp.errorMessage = verifyDetails;
        return resp;
    }

    if (req.command == protocol::CMD_SUSPEND_PROCESS) {
        if (req.args.empty()) {
            resp.success = false;
            resp.errorMessage = "Missing PID argument";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        ControlResult res = controller_.suspendProcess(pid);
        resp.success = res.success;
        resp.dataJson = "{\"pid\":" + std::to_string(pid) + ",\"action\":\"SUSPEND\",\"duration_ms\":" +
                        std::to_string(res.durationMs) + "}";
        if (!res.success) resp.errorMessage = res.errorMessage;
        return resp;
    }

    if (req.command == protocol::CMD_RESUME_PROCESS) {
        if (req.args.empty()) {
            resp.success = false;
            resp.errorMessage = "Missing PID argument";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        ControlResult res = controller_.resumeProcess(pid);
        resp.success = res.success;
        resp.dataJson = "{\"pid\":" + std::to_string(pid) + ",\"action\":\"RESUME\",\"duration_ms\":" +
                        std::to_string(res.durationMs) + "}";
        if (!res.success) resp.errorMessage = res.errorMessage;
        return resp;
    }

    if (req.command == protocol::CMD_CHANGE_PRIORITY) {
        if (req.args.size() < 2) {
            resp.success = false;
            resp.errorMessage = "Usage: CHANGE_PRIORITY <pid> <nice>";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        int nice = std::stoi(req.args[1]);
        ControlResult res = controller_.changePriority(pid, nice);
        resp.success = res.success;
        resp.dataJson = "{\"pid\":" + std::to_string(pid) + ",\"priority\":" + std::to_string(nice) +
                        ",\"duration_ms\":" + std::to_string(res.durationMs) + "}";
        if (!res.success) resp.errorMessage = res.errorMessage;
        return resp;
    }

    if (req.command == protocol::CMD_RESTART_PROCESS) {
        if (req.args.empty()) {
            resp.success = false;
            resp.errorMessage = "Usage: RESTART_PROCESS <pid> [commandLine...]";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        std::string cmd;
        if (req.args.size() > 1) {
            for (size_t i = 1; i < req.args.size(); ++i) {
                if (i > 1) cmd += " ";
                cmd += req.args[i];
            }
        } else {
            ProcessIdentity identity;
            if (ProcessRegistry::instance().getRegisteredInfo(pid, identity)) {
                cmd = identity.commandLine;
            }
        }

        int newPid = 0;
        ControlResult res = controller_.restartProcess(pid, cmd, newPid);
        std::string verifyDetails;
        bool verified = false;
        if (res.success && newPid > 0) {
            verified = verification_.verifyRestart(newPid, 100, verifyDetails);
            if (verified) {
                ProcessRegistry::instance().registerProcess(newPid, "restarted_proc", cmd);
            }
        }

        std::ostringstream ss;
        ss << "{\"old_pid\":" << pid
           << ",\"new_pid\":" << newPid
           << ",\"verified\":" << (verified ? "true" : "false")
           << ",\"details\":\"" << utils::escapeJson(verifyDetails) << "\"}";
        resp.success = verified;
        resp.dataJson = ss.str();
        if (!verified) resp.errorMessage = verifyDetails.empty() ? res.errorMessage : verifyDetails;
        return resp;
    }

    if (req.command == protocol::CMD_VERIFY_PROCESS) {
        if (req.args.empty()) {
            resp.success = false;
            resp.errorMessage = "Missing PID argument";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        bool alive = utils::isPidAlive(pid);
        resp.success = true;
        resp.dataJson = "{\"pid\":" + std::to_string(pid) + ",\"is_alive\":" + (alive ? "true" : "false") + "}";
        return resp;
    }

    if (req.command == protocol::CMD_CHECK_DEADLOCK) {
        std::string path = (req.args.empty()) ? "/tmp/oscope_deadlock.state" : req.args[0];
        DeadlockGraph graph = deadlockDetector_.analyzeFromStateFile(path);

        std::ostringstream ss;
        ss << "{\"has_deadlock\":" << (graph.hasDeadlock ? "true" : "false")
           << ",\"process_pid\":" << graph.processPid
           << ",\"victim_thread\":" << graph.victimThreadId
           << ",\"strategy\":\"" << utils::escapeJson(graph.strategyUsed) << "\""
           << ",\"cycle\":[";
        for (size_t i = 0; i < graph.cycleThreadIds.size(); ++i) {
            ss << graph.cycleThreadIds[i];
            if (i + 1 < graph.cycleThreadIds.size()) ss << ",";
        }
        ss << "],\"edges\":[";
        for (size_t i = 0; i < graph.edges.size(); ++i) {
            ss << "{\"waiting_thread\":" << graph.edges[i].waitingThreadId
               << ",\"holding_thread\":" << graph.edges[i].holdingThreadId
               << ",\"resource\":\"" << utils::escapeJson(graph.edges[i].resourceName) << "\"}";
            if (i + 1 < graph.edges.size()) ss << ",";
        }
        ss << "]}";

        resp.success = true;
        resp.dataJson = ss.str();
        return resp;
    }

    if (req.command == protocol::CMD_REGISTER_PROCESS) {
        if (req.args.size() < 2) {
            resp.success = false;
            resp.errorMessage = "Usage: REGISTER_PROCESS <pid> <name> [cmdLine...]";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        std::string name = req.args[1];
        std::string cmd;
        for (size_t i = 2; i < req.args.size(); ++i) {
            if (i > 2) cmd += " ";
            cmd += req.args[i];
        }

        ProcessRegistry::instance().registerProcess(pid, name, cmd);
        resp.success = true;
        resp.dataJson = "{\"pid\":" + std::to_string(pid) + ",\"registered\":true}";
        return resp;
    }

    if (req.command == protocol::CMD_UNREGISTER_PROCESS) {
        if (req.args.empty()) {
            resp.success = false;
            resp.errorMessage = "Missing PID argument";
            return resp;
        }
        int pid = std::stoi(req.args[0]);
        ProcessRegistry::instance().unregisterProcess(pid);
        resp.success = true;
        resp.dataJson = "{\"pid\":" + std::to_string(pid) + ",\"registered\":false}";
        return resp;
    }

    if (req.command == protocol::CMD_GET_SYSTEM_HEALTH) {
        SystemMetrics sys;
        monitor_.getMemoryMonitor().readSystemMemory(sys);
        sys.cpuUsageTotal = monitor_.getCpuCalculator().calculateSystemCpu();

        std::string health = "HEALTHY";
        if (sys.cpuUsageTotal >= 85.0 || sys.memoryPercent >= 85.0) {
            health = "CRITICAL";
        } else if (sys.cpuUsageTotal >= 70.0 || sys.memoryPercent >= 70.0) {
            health = "WARNING";
        }

        std::ostringstream ss;
        ss << "{\"health\":\"" << health << "\""
           << ",\"cpu_total\":" << sys.cpuUsageTotal
           << ",\"memory_percent\":" << sys.memoryPercent
           << ",\"memory_total_mb\":" << sys.memoryTotalMb
           << ",\"memory_used_mb\":" << sys.memoryUsedMb
           << ",\"memory_free_mb\":" << sys.memoryFreeMb
           << ",\"swap_used_mb\":" << sys.swapUsedMb
           << "}";
        resp.success = true;
        resp.dataJson = ss.str();
        return resp;
    }

    if (req.command == protocol::CMD_GET_ALL_PROCESSES) {
        auto procs = monitor_.scanAllProcesses();
        std::ostringstream ss;
        ss << "[";
        size_t count = 0;
        for (size_t i = 0; i < procs.size() && count < 100; ++i) {
            const auto& p = procs[i];
            if (count > 0) ss << ",";
            ss << "{\"pid\":" << p.identity.pid
               << ",\"name\":\"" << utils::escapeJson(p.identity.processName) << "\""
               << ",\"ppid\":" << p.ppid
               << ",\"state\":\"" << processStateToString(p.currentMetrics.state) << "\""
               << ",\"cpu_usage\":" << p.currentMetrics.cpuUsagePercent
               << ",\"memory_rss_mb\":" << p.currentMetrics.memoryRssMb
               << ",\"threads\":" << p.currentMetrics.threadCount
               << ",\"priority\":" << p.currentMetrics.priority
               << ",\"open_fds\":" << p.currentMetrics.openFileDescriptors
               << ",\"is_registered\":" << (ProcessRegistry::instance().isRegistered(p.identity.pid) ? "true" : "false")
               << "}";
            count++;
        }
        ss << "]";
        resp.success = true;
        resp.dataJson = ss.str();
        return resp;
    }

    resp.success = false;
    resp.errorMessage = "Unknown command: " + req.command;
    return resp;
}

} // namespace oscope
