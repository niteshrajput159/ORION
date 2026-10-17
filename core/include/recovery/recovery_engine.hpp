#pragma once

#include "common/types.hpp"
#include "common/config.hpp"
#include "process/process_controller.hpp"
#include "process/process_registry.hpp"
#include "verification/verification_engine.hpp"
#include "recovery/adaptive_recovery.hpp"
#include "recovery/crash_recovery.hpp"
#include "recovery/crash_loop_recovery.hpp"
#include "recovery/hang_recovery.hpp"
#include <vector>
#include <map>

namespace oscope {

class RecoveryEngine {
public:
    RecoveryEngine(const Config& config,
                   ProcessController& controller,
                   VerificationEngine& verification);

    // Loads policy rules from configured definitions
    void loadPolicies(const std::vector<PolicyRule>& policies);

    // Matches best policy for an incident
    bool matchPolicy(IncidentType type, IncidentSeverity severity, PolicyRule& outPolicy) const;

    // Executes end-to-end recovery for an incident
    RecoveryRecord executeRecovery(const IncidentRecord& incident,
                                  const std::string& commandLine,
                                  int currentAttemptNumber,
                                  int& outNewPid);

    AdaptiveRecoveryScorer& getAdaptiveScorer() { return adaptiveScorer_; }
    ProcessController& getController() { return controller_; }
    VerificationEngine& getVerification() { return verification_; }

private:
    Config config_;
    ProcessController& controller_;
    VerificationEngine& verification_;
    AdaptiveRecoveryScorer adaptiveScorer_;
    CrashRecoveryHandler crashRecovery_;
    CrashLoopRecoveryHandler crashLoopRecovery_;
    HangRecoveryHandler hangRecovery_;
    std::vector<PolicyRule> policies_;
};

} // namespace oscope
