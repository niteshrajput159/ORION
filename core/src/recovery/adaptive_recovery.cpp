#include "recovery/adaptive_recovery.hpp"

namespace oscope {

void AdaptiveRecoveryScorer::recordOutcome(RecoveryAction action, bool success) {
    auto& stat = history_[action];
    stat.totalAttempts++;
    if (success) {
        stat.successfulAttempts++;
    }
}

double AdaptiveRecoveryScorer::getSuccessProbability(RecoveryAction action) const {
    auto it = history_.find(action);
    if (it == history_.end()) {
        // Unobserved action: Laplace prior 1/2 = 0.5
        return 0.5;
    }
    const auto& stat = it->second;
    return static_cast<double>(stat.successfulAttempts + 1) / static_cast<double>(stat.totalAttempts + 2);
}

RecoveryAction AdaptiveRecoveryScorer::selectBestAction(const std::vector<RecoveryAction>& candidates, RecoveryAction defaultAction) const {
    if (candidates.empty()) return defaultAction;

    RecoveryAction bestAction = defaultAction;
    double bestScore = -1.0;

    for (auto act : candidates) {
        double score = getSuccessProbability(act);
        if (score > bestScore) {
            bestScore = score;
            bestAction = act;
        }
    }

    return bestAction;
}

void AdaptiveRecoveryScorer::clear() {
    history_.clear();
}

} // namespace oscope
