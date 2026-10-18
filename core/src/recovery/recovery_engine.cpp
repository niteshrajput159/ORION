#include "recovery/recovery_engine.hpp"
#include "common/utils.hpp"
#include <iostream>

namespace oscope {

RecoveryEngine::RecoveryEngine(const Config& config,
                               ProcessController& controller,
                               VerificationEngine& verification)
    : config_(config),
      controller_(controller),
      verification_(verification),
      adaptiveScorer_(),
      crashRecovery_(controller, verification),
      crashLoopRecovery_(config.maxRestartAttempts),
      hangRecovery_(controller, verification) {}

void RecoveryEngine::loadPolicies(const std::vector<PolicyRule>& policies) {
    policies_ = policies;
}

bool RecoveryEngine::matchPolicy(IncidentType type, IncidentSeverity severity, PolicyRule& outPolicy) const {
    // Exact match on type & severity first
    for (const auto& pol : policies_) {
        if (!pol.enabled) continue;
        if (pol.incidentType == type && pol.severity == severity) {
            outPolicy = pol;
            return true;
        }
    }

    // Fallback: match on type
    for (const auto& pol : policies_) {
        if (!pol.enabled) continue;
        if (pol.incidentType == type) {
            outPolicy = pol;
            return true;
        }
    }

    // Default policy per incident type
    outPolicy.incidentType = type;
    outPolicy.severity = severity;
    outPolicy.enabled = true;
    outPolicy.maxAttempts = config_.maxRestartAttempts;
    outPolicy.timeoutSeconds = config_.sigtermTimeoutSeconds;

    switch (type) {
        case IncidentType::HIGH_CPU:
            outPolicy.action = RecoveryAction::TERMINATE_SIGTERM;
            break;
        case IncidentType::HIGH_MEMORY:
            outPolicy.action = RecoveryAction::TERMINATE_SIGTERM;
            break;
        case IncidentType::PROCESS_CRASH:
            outPolicy.action = RecoveryAction::RESTART;
            break;
        case IncidentType::CRASH_LOOP:
            outPolicy.action = RecoveryAction::ALERT_ONLY;
            break;
        case IncidentType::HUNG_PROCESS:
            outPolicy.action = RecoveryAction::TERMINATE_SIGTERM;
            break;
        case IncidentType::DEADLOCK:
            outPolicy.action = RecoveryAction::RESOLVE_DEADLOCK;
            break;
        case IncidentType::EXCESSIVE_THREADS:
            outPolicy.action = RecoveryAction::TERMINATE_SIGTERM;
            break;
        case IncidentType::EXCESSIVE_FILE_DESCRIPTORS:
            outPolicy.action = RecoveryAction::TERMINATE_SIGTERM;
            break;
        case IncidentType::HIGH_IO:
            outPolicy.action = RecoveryAction::TERMINATE_SIGTERM;
            break;
        default:
            outPolicy.action = RecoveryAction::ALERT_ONLY;
            break;
    }

    return true;
}

RecoveryRecord RecoveryEngine::executeRecovery(const IncidentRecord& incident,
                                              const std::string& commandLine,
                                              int currentAttemptNumber,
                                              int& outNewPid) {
    outNewPid = 0;
    RecoveryRecord rec;
    rec.incidentId = incident.incidentId;
    rec.attemptNumber = currentAttemptNumber;
    rec.startedAtIso = utils::getCurrentIsoTime();
    uint64_t startMs = utils::getMonotonicTimeMs();

    // 1. Match Policy
    PolicyRule matchedPolicy;
    matchPolicy(incident.type, incident.severity, matchedPolicy);
    rec.action = matchedPolicy.action;

    // Safety checks
    bool permitted = ProcessRegistry::instance().isActionPermitted(
        incident.pid, incident.processName, config_.onlyRegisteredProcesses);

    if (!permitted) {
        rec.success = false;
        rec.result = RecoveryResult::VERIFIED_FAILURE;
        rec.verificationDetails = "SAFETY VIOLATION PREVENTED: Process " + incident.processName +
                                 " (PID " + std::to_string(incident.pid) +
                                 ") is not a registered test workload. Automated recovery blocked.";
        rec.completedAtIso = utils::getCurrentIsoTime();
        rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
        return rec;
    }

    // Dry Run check
    if (config_.dryRun || !config_.allowProcessControl) {
        rec.success = true;
        rec.result = RecoveryResult::VERIFIED_SUCCESS;
        rec.verificationDetails = "DRY-RUN MODE: Action " + actionToString(matchedPolicy.action) +
                                 " simulated successfully without modifying OS state.";
        rec.completedAtIso = utils::getCurrentIsoTime();
        rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
        return rec;
    }

    // 2. Dispatch Action based on type
    switch (matchedPolicy.action) {
        case RecoveryAction::RESTART: {
            // Check if bounds exceeded
            if (!crashLoopRecovery_.shouldAttemptRestart(currentAttemptNumber, matchedPolicy.maxAttempts)) {
                rec = crashLoopRecovery_.handleExceededLimit(incident.incidentId, currentAttemptNumber);
                adaptiveScorer_.recordOutcome(RecoveryAction::RESTART, false);
                return rec;
            }

            rec = crashRecovery_.handleCrash(incident.pid, commandLine, currentAttemptNumber, outNewPid);
            adaptiveScorer_.recordOutcome(RecoveryAction::RESTART, rec.success);
            return rec;
        }

        case RecoveryAction::TERMINATE_SIGTERM: {
            rec = hangRecovery_.handleHungProcess(incident.pid, matchedPolicy.timeoutSeconds);
            adaptiveScorer_.recordOutcome(RecoveryAction::TERMINATE_SIGTERM, rec.success);
            return rec;
        }

        case RecoveryAction::TERMINATE_SIGKILL: {
            ControlResult killRes = controller_.terminateSigkill(incident.pid);
            bool dummy = false;
            std::string verifyDetails;
            bool verified = verification_.verifyTermination(incident.pid, 2, dummy, verifyDetails);

            rec.success = verified;
            rec.result = verified ? RecoveryResult::VERIFIED_SUCCESS : RecoveryResult::VERIFIED_FAILURE;
            rec.verificationDetails = "SIGKILL executed: " + verifyDetails;
            rec.completedAtIso = utils::getCurrentIsoTime();
            rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
            adaptiveScorer_.recordOutcome(RecoveryAction::TERMINATE_SIGKILL, rec.success);
            return rec;
        }

        case RecoveryAction::LOWER_PRIORITY: {
            ControlResult prioRes = controller_.changePriority(incident.pid, 10);
            rec.success = prioRes.success;
            rec.result = prioRes.success ? RecoveryResult::VERIFIED_SUCCESS : RecoveryResult::VERIFIED_FAILURE;
            rec.verificationDetails = prioRes.success
                ? "Priority successfully throttled (nice +10)."
                : "Failed to alter process priority: " + prioRes.errorMessage;
            rec.completedAtIso = utils::getCurrentIsoTime();
            rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
            adaptiveScorer_.recordOutcome(RecoveryAction::LOWER_PRIORITY, rec.success);
            return rec;
        }

        case RecoveryAction::SUSPEND: {
            ControlResult suspRes = controller_.suspendProcess(incident.pid);
            rec.success = suspRes.success;
            rec.result = suspRes.success ? RecoveryResult::VERIFIED_SUCCESS : RecoveryResult::VERIFIED_FAILURE;
            rec.verificationDetails = suspRes.success
                ? "Process execution suspended (SIGSTOP)."
                : "Failed to suspend process: " + suspRes.errorMessage;
            rec.completedAtIso = utils::getCurrentIsoTime();
            rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
            return rec;
        }

        case RecoveryAction::RESUME: {
            ControlResult resRes = controller_.resumeProcess(incident.pid);
            rec.success = resRes.success;
            rec.result = resRes.success ? RecoveryResult::VERIFIED_SUCCESS : RecoveryResult::VERIFIED_FAILURE;
            rec.verificationDetails = resRes.success
                ? "Process execution resumed (SIGCONT)."
                : "Failed to resume process: " + resRes.errorMessage;
            rec.completedAtIso = utils::getCurrentIsoTime();
            rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
            return rec;
        }

        case RecoveryAction::RESOLVE_DEADLOCK: {
            // If metadata contains victim thread, terminate that thread/process
            int victimId = 0;
            auto it = incident.metadata.find("victim_thread");
            if (it != incident.metadata.end()) {
                try { victimId = std::stoi(it->second); } catch (...) {}
            }
            if (victimId == 0) victimId = incident.pid;

            ControlResult killRes = controller_.terminateSigkill(victimId);
            rec.success = killRes.success;
            rec.result = killRes.success ? RecoveryResult::VERIFIED_SUCCESS : RecoveryResult::VERIFIED_FAILURE;
            rec.verificationDetails = "Terminated deadlock cycle victim " + std::to_string(victimId) + ". Cycle eliminated.";
            rec.completedAtIso = utils::getCurrentIsoTime();
            rec.durationMs = static_cast<double>(utils::getMonotonicTimeMs() - startMs);
            adaptiveScorer_.recordOutcome(RecoveryAction::RESOLVE_DEADLOCK, rec.success);
            return rec;
        }

        case RecoveryAction::ALERT_ONLY:
        default: {
            rec.success = true;
            rec.result = RecoveryResult::VERIFIED_SUCCESS;
            rec.verificationDetails = "Alert-only policy: no destructive recovery executed.";
            rec.completedAtIso = utils::getCurrentIsoTime();
            rec.durationMs = 0.0;
            return rec;
        }
    }
}

} // namespace oscope
