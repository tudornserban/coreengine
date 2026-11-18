#pragma once

#include "core/ai/DecisionGraph.h"
#include "core/interfaces/IDecisionSnapshot.h"
#include "core/math/DeterministicRNG.h"

#include <string>
#include <vector>

namespace CoreEngine {

struct DecisionEvalResult {
    int primaryActionId = 0;
    int secondaryActionId = -1;
    bool defaultedOnGuard = false;
    int stepsExecuted = 0;
    std::vector<std::string> visitedNodeIds;
};

class DecisionGraphInterpreter {
public:
    static constexpr int kDefaultMaxSteps = 100;

    explicit DecisionGraphInterpreter(int maxSteps = kDefaultMaxSteps, int fallbackActionId = 0);

    DecisionEvalResult Evaluate(const DecisionGraph& graph, const IDecisionSnapshot& snapshot,
                                DeterministicRNG& rng, int turnCounter = 0) const;

private:
    int maxSteps_;
    int fallbackActionId_;
};

} // namespace CoreEngine