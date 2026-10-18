#pragma once

#include "common/types.hpp"
#include <map>
#include <vector>
#include <string>

namespace oscope {

class AdaptiveRecoveryScorer {
public:
    AdaptiveRecoveryScorer() = default;

    // Records historical outcome for an action
    void recordOutcome(RecoveryAction action, bool success);

    // Calculates Laplace-smoothed empirical success probability:
    // P(success) = (successes + 1) / (total + 2)
    double getSuccessProbability(RecoveryAction action) const;

    // Evaluates a list of candidate actions and selects the one with the highest historical success score
    RecoveryAction selectBestAction(const std::vector<RecoveryAction>& candidates, RecoveryAction defaultAction) const;

    void clear();

private:
    struct ActionStat {
        int totalAttempts{0};
        int successfulAttempts{0};
    };

    std::map<RecoveryAction, ActionStat> history_;
};

} // namespace oscope
