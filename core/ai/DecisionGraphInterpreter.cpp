#include "core/ai/DecisionGraphInterpreter.h"

#include "core/logging/Logger.h"

#include <cmath>

namespace CoreEngine {
namespace {

bool CompareScalar(float left, float right, ComparisonOp op) {
    switch (op) {
    case ComparisonOp::LT:
        return left < right;
    case ComparisonOp::LTE:
        return left <= right;
    case ComparisonOp::EQ:
        return left == right;
    case ComparisonOp::NEQ:
        return left != right;
    case ComparisonOp::GTE:
        return left >= right;
    case ComparisonOp::GT:
        return left > right;
    }
    return false;
}

bool EvaluateCondition(const DecisionNode& node, const IDecisionSnapshot& snapshot) {
    switch (node.valueKind) {
    case DecisionValueKind::Float: {
        const float value = snapshot.GetFloatProperty(node.target, node.propertyKey, 0.0f);
        return CompareScalar(value, node.floatThreshold, node.op);
    }
    case DecisionValueKind::Int: {
        const int value = snapshot.GetIntProperty(node.target, node.propertyKey, 0);
        return CompareScalar(static_cast<float>(value), static_cast<float>(node.intThreshold), node.op);
    }
    case DecisionValueKind::Bool: {
        const bool value = snapshot.GetBoolProperty(node.target, node.propertyKey, false);
        const bool threshold = node.boolThreshold;
        switch (node.op) {
        case ComparisonOp::EQ:
            return value == threshold;
        case ComparisonOp::NEQ:
            return value != threshold;
        default:
            return CompareScalar(value ? 1.0f : 0.0f, threshold ? 1.0f : 0.0f, node.op);
        }
    }
    }
    return false;
}

DecisionEvalResult MakeGuardResult(int fallbackActionId, int steps,
                                   std::vector<std::string> visited, const char* reason) {
    LOG_WARN("DecisionGraphInterpreter: %s", reason);
    DecisionEvalResult result;
    result.primaryActionId = fallbackActionId;
    result.secondaryActionId = -1;
    result.defaultedOnGuard = true;
    result.stepsExecuted = steps;
    result.visitedNodeIds = std::move(visited);
    return result;
}

int PickSelectorPortCycle(const std::vector<int>& ports, int turnCounter) {
    if (ports.empty()) {
        return -1;
    }
    const std::size_t index = static_cast<std::size_t>(turnCounter % static_cast<int>(ports.size()));
    return ports[index];
}

int PickSelectorPortWeighted(const DecisionNode& node, const std::vector<int>& ports, DeterministicRNG& rng) {
    if (ports.empty()) {
        return -1;
    }
    float totalWeight = 0.0f;
    for (int port : ports) {
        if (port >= 0 && static_cast<std::size_t>(port) < node.weights.size()) {
            const float w = node.weights[static_cast<std::size_t>(port)];
            if (w > 0.0f) {
                totalWeight += w;
            }
        }
    }
    if (totalWeight <= 0.0f) {
        return ports.front();
    }
    const float draw = rng.NextFloat() * totalWeight;
    float cumulative = 0.0f;
    for (int port : ports) {
        float w = 0.0f;
        if (port >= 0 && static_cast<std::size_t>(port) < node.weights.size()) {
            w = std::max(0.0f, node.weights[static_cast<std::size_t>(port)]);
        }
        cumulative += w;
        if (draw < cumulative) {
            return port;
        }
    }
    return ports.back();
}

} // namespace

DecisionGraphInterpreter::DecisionGraphInterpreter(int maxSteps, int fallbackActionId)
    : maxSteps_(maxSteps), fallbackActionId_(fallbackActionId) {}

DecisionEvalResult DecisionGraphInterpreter::Evaluate(const DecisionGraph& graph,
                                                      const IDecisionSnapshot& snapshot, DeterministicRNG& rng,
                                                      int turnCounter) const {
    std::vector<std::string> visited;
    const std::string entryId = graph.FindEntryNodeId();
    if (entryId.empty()) {
        return MakeGuardResult(fallbackActionId_, 0, std::move(visited), "graph has no Entry node");
    }

    std::string currentId = entryId;
    int steps = 0;

    while (steps < maxSteps_) {
        const DecisionNode* node = graph.FindNode(currentId);
        if (!node) {
            return MakeGuardResult(fallbackActionId_, steps, std::move(visited),
                                   "current node id not found in graph");
        }

        visited.push_back(currentId);
        ++steps;

        if (node->type == DecisionNodeType::Action) {
            DecisionEvalResult result;
            result.primaryActionId = node->primaryActionId;
            result.secondaryActionId = node->secondaryActionId;
            result.defaultedOnGuard = false;
            result.stepsExecuted = steps;
            result.visitedNodeIds = std::move(visited);
            return result;
        }

        int outPort = 0;
        if (node->type == DecisionNodeType::Entry) {
            outPort = 0;
        } else if (node->type == DecisionNodeType::Condition) {
            const bool passed = EvaluateCondition(*node, snapshot);
            outPort = passed ? 0 : 1;
        } else if (node->type == DecisionNodeType::Selector) {
            const std::vector<int> ports = graph.GetSortedConnectedOutPorts(currentId);
            if (node->selectorMode == SelectorMode::Cycle) {
                outPort = PickSelectorPortCycle(ports, turnCounter);
            } else {
                outPort = PickSelectorPortWeighted(*node, ports, rng);
            }
            if (outPort < 0) {
                return MakeGuardResult(fallbackActionId_, steps, std::move(visited),
                                       "Selector has no connected output ports");
            }
        }

        const DecisionConnection* edge = graph.FindOutgoingConnection(currentId, outPort);
        if (!edge) {
            return MakeGuardResult(fallbackActionId_, steps, std::move(visited),
                                   "dead end: no connection on selected output port");
        }
        currentId = edge->toNodeId;
    }

    return MakeGuardResult(fallbackActionId_, steps, std::move(visited),
                           "exceeded max interpreter steps (possible cycle)");
}

} // namespace CoreEngine