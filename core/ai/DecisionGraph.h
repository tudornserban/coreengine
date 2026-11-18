#pragma once

#include "core/interfaces/IDecisionSnapshot.h"

#include <cstdint>
#include <string>
#include <vector>

namespace CoreEngine {

enum class DecisionNodeType : uint8_t { Entry = 0, Condition = 1, Selector = 2, Action = 3 };

enum class DecisionValueKind : uint8_t { Float = 0, Int = 1, Bool = 2 };

enum class ComparisonOp : uint8_t { LT = 0, LTE = 1, EQ = 2, NEQ = 3, GTE = 4, GT = 5 };

enum class SelectorMode : uint8_t { Cycle = 0, Weighted = 1 };

struct DecisionOutcome {
    int primaryActionId = 0;
    int secondaryActionId = -1;
    std::string actionTag;
};

struct DecisionConnection {
    std::string fromNodeId;
    int fromPort = 0;
    std::string toNodeId;
    int toPort = 0;
};

struct DecisionNode {
    std::string id;
    DecisionNodeType type = DecisionNodeType::Entry;

    DecisionTarget target = DecisionTarget::Self;
    DecisionValueKind valueKind = DecisionValueKind::Float;
    std::string propertyKey;
    ComparisonOp op = ComparisonOp::LT;
    float floatThreshold = 0.0f;
    int intThreshold = 0;
    bool boolThreshold = false;

    SelectorMode selectorMode = SelectorMode::Cycle;
    std::vector<float> weights;

    int primaryActionId = 0;
    int secondaryActionId = -1;
    std::string actionTag;
};

/// Directed decision graph: Entry / Condition / Selector / Action nodes with port-indexed edges.
class DecisionGraph {
public:
    void Clear();

    void AddNode(DecisionNode node);
    const DecisionNode* FindNode(const std::string& id) const;

    bool AddConnection(const DecisionConnection& connection, std::string* outError = nullptr);
    bool ValidateConnection(const std::string& fromId, int fromPort, const std::string& toId, int toPort,
                            std::string* outError = nullptr) const;

    const std::vector<DecisionNode>& Nodes() const { return nodes_; }
    const std::vector<DecisionConnection>& Connections() const { return connections_; }

    std::string FindEntryNodeId() const;
    const DecisionConnection* FindOutgoingConnection(const std::string& fromNodeId, int fromPort) const;
    std::vector<int> GetSortedConnectedOutPorts(const std::string& fromNodeId) const;

    std::string ToJsonString() const;
    static bool FromJsonString(const std::string& json, DecisionGraph* outGraph,
                               std::string* outError = nullptr);

private:
    std::vector<DecisionNode> nodes_;
    std::vector<DecisionConnection> connections_;
};

} // namespace CoreEngine