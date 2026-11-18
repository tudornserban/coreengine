#include <gtest/gtest.h>

#include "core/ai/DecisionGraph.h"
#include "core/ai/DecisionGraphInterpreter.h"
#include "core/interfaces/IDecisionSnapshot.h"
#include "core/math/DeterministicRNG.h"

#include <unordered_map>

namespace {

class MapDecisionSnapshot final : public CoreEngine::IDecisionSnapshot {
public:
    float GetFloatProperty(CoreEngine::DecisionTarget target, const std::string& key,
                           float defaultValue) const override {
        const auto tk = Key{target, key};
        const auto it = floats_.find(tk);
        return it != floats_.end() ? it->second : defaultValue;
    }

    int GetIntProperty(CoreEngine::DecisionTarget target, const std::string& key, int defaultValue) const override {
        const auto tk = Key{target, key};
        const auto it = ints_.find(tk);
        return it != ints_.end() ? it->second : defaultValue;
    }

    bool GetBoolProperty(CoreEngine::DecisionTarget target, const std::string& key,
                         bool defaultValue) const override {
        const auto tk = Key{target, key};
        const auto it = bools_.find(tk);
        return it != bools_.end() ? it->second : defaultValue;
    }

    void SetFloat(CoreEngine::DecisionTarget target, const std::string& key, float value) {
        floats_[Key{target, key}] = value;
    }

    void SetBool(CoreEngine::DecisionTarget target, const std::string& key, bool value) {
        bools_[Key{target, key}] = value;
    }

private:
    struct Key {
        CoreEngine::DecisionTarget target;
        std::string key;
        bool operator==(const Key& other) const { return target == other.target && key == other.key; }
    };

    struct KeyHash {
        std::size_t operator()(const Key& k) const {
            return std::hash<std::string>{}(k.key) ^ (static_cast<int>(k.target) << 1);
        }
    };

    std::unordered_map<Key, float, KeyHash> floats_;
    std::unordered_map<Key, int, KeyHash> ints_;
    std::unordered_map<Key, bool, KeyHash> bools_;
};

CoreEngine::DecisionGraph BuildEntryConditionActionGraph() {
    CoreEngine::DecisionGraph graph;
    graph.AddNode(CoreEngine::DecisionNode{.id = "entry", .type = CoreEngine::DecisionNodeType::Entry});

    CoreEngine::DecisionNode condition;
    condition.id = "hp_check";
    condition.type = CoreEngine::DecisionNodeType::Condition;
    condition.target = CoreEngine::DecisionTarget::Self;
    condition.valueKind = CoreEngine::DecisionValueKind::Float;
    condition.propertyKey = "hp_pct";
    condition.op = CoreEngine::ComparisonOp::GTE;
    condition.floatThreshold = 0.5f;
    graph.AddNode(condition);

    CoreEngine::DecisionNode heavy;
    heavy.id = "heavy";
    heavy.type = CoreEngine::DecisionNodeType::Action;
    heavy.primaryActionId = 10;
    heavy.secondaryActionId = 2;
    heavy.actionTag = "heavy";
    graph.AddNode(heavy);

    CoreEngine::DecisionNode retreat;
    retreat.id = "retreat";
    retreat.type = CoreEngine::DecisionNodeType::Action;
    retreat.primaryActionId = 20;
    retreat.actionTag = "retreat";
    graph.AddNode(retreat);

    std::string err;
    graph.AddConnection({"entry", 0, "hp_check", 0}, &err);
    graph.AddConnection({"hp_check", 0, "heavy", 0}, &err);
    graph.AddConnection({"hp_check", 1, "retreat", 0}, &err);
    return graph;
}

} // namespace

TEST(DecisionGraphTest, EntryConditionActionTrueBranch) {
    const CoreEngine::DecisionGraph graph = BuildEntryConditionActionGraph();
    MapDecisionSnapshot snapshot;
    snapshot.SetFloat(CoreEngine::DecisionTarget::Self, "hp_pct", 0.75f);

    DeterministicRNG rng(1);
    CoreEngine::DecisionGraphInterpreter interpreter;
    const CoreEngine::DecisionEvalResult result = interpreter.Evaluate(graph, snapshot, rng);

    EXPECT_FALSE(result.defaultedOnGuard);
    EXPECT_EQ(result.primaryActionId, 10);
    EXPECT_EQ(result.secondaryActionId, 2);
    ASSERT_EQ(result.visitedNodeIds.size(), 3u);
    EXPECT_EQ(result.visitedNodeIds[0], "entry");
    EXPECT_EQ(result.visitedNodeIds[1], "hp_check");
    EXPECT_EQ(result.visitedNodeIds[2], "heavy");
}

TEST(DecisionGraphTest, MissingFalseBranchDefaultsOnGuard) {
    CoreEngine::DecisionGraph graph;
    graph.AddNode(CoreEngine::DecisionNode{.id = "entry", .type = CoreEngine::DecisionNodeType::Entry});

    CoreEngine::DecisionNode condition;
    condition.id = "hp_check";
    condition.type = CoreEngine::DecisionNodeType::Condition;
    condition.valueKind = CoreEngine::DecisionValueKind::Float;
    condition.propertyKey = "hp_pct";
    condition.op = CoreEngine::ComparisonOp::GTE;
    condition.floatThreshold = 0.5f;
    graph.AddNode(condition);

    graph.AddNode(
        CoreEngine::DecisionNode{.id = "ok", .type = CoreEngine::DecisionNodeType::Action, .primaryActionId = 5});

    std::string err;
    graph.AddConnection({"entry", 0, "hp_check", 0}, &err);
    graph.AddConnection({"hp_check", 0, "ok", 0}, &err);

    MapDecisionSnapshot snapshot;
    snapshot.SetFloat(CoreEngine::DecisionTarget::Self, "hp_pct", 0.1f);

    DeterministicRNG rng(1);
    CoreEngine::DecisionGraphInterpreter interpreter(100, 99);
    const CoreEngine::DecisionEvalResult result = interpreter.Evaluate(graph, snapshot, rng);

    EXPECT_TRUE(result.defaultedOnGuard);
    EXPECT_EQ(result.primaryActionId, 99);
    EXPECT_EQ(result.secondaryActionId, -1);
}

TEST(DecisionGraphTest, SelectorCycleDeterministicByTurnCounter) {
    CoreEngine::DecisionGraph graph;
    graph.AddNode(CoreEngine::DecisionNode{.id = "entry", .type = CoreEngine::DecisionNodeType::Entry});

    CoreEngine::DecisionNode selector;
    selector.id = "pick";
    selector.type = CoreEngine::DecisionNodeType::Selector;
    selector.selectorMode = CoreEngine::SelectorMode::Cycle;
    graph.AddNode(selector);

    graph.AddNode(
        CoreEngine::DecisionNode{.id = "a", .type = CoreEngine::DecisionNodeType::Action, .primaryActionId = 1});
    graph.AddNode(
        CoreEngine::DecisionNode{.id = "b", .type = CoreEngine::DecisionNodeType::Action, .primaryActionId = 2});
    graph.AddNode(
        CoreEngine::DecisionNode{.id = "c", .type = CoreEngine::DecisionNodeType::Action, .primaryActionId = 3});

    std::string err;
    graph.AddConnection({"entry", 0, "pick", 0}, &err);
    graph.AddConnection({"pick", 0, "a", 0}, &err);
    graph.AddConnection({"pick", 1, "b", 0}, &err);
    graph.AddConnection({"pick", 2, "c", 0}, &err);

    MapDecisionSnapshot snapshot;
    DeterministicRNG rng(42);
    CoreEngine::DecisionGraphInterpreter interpreter;

    const CoreEngine::DecisionEvalResult r0 = interpreter.Evaluate(graph, snapshot, rng, 0);
    const CoreEngine::DecisionEvalResult r1 = interpreter.Evaluate(graph, snapshot, rng, 1);
    const CoreEngine::DecisionEvalResult r3 = interpreter.Evaluate(graph, snapshot, rng, 3);

    EXPECT_EQ(r0.primaryActionId, 1);
    EXPECT_EQ(r1.primaryActionId, 2);
    EXPECT_EQ(r3.primaryActionId, 1);
}

TEST(DecisionGraphTest, SelectorWeightedUsesDeterministicRng) {
    CoreEngine::DecisionGraph graph;
    graph.AddNode(CoreEngine::DecisionNode{.id = "entry", .type = CoreEngine::DecisionNodeType::Entry});

    CoreEngine::DecisionNode selector;
    selector.id = "pick";
    selector.type = CoreEngine::DecisionNodeType::Selector;
    selector.selectorMode = CoreEngine::SelectorMode::Weighted;
    selector.weights = {1.0f, 3.0f};
    graph.AddNode(selector);

    graph.AddNode(
        CoreEngine::DecisionNode{.id = "a", .type = CoreEngine::DecisionNodeType::Action, .primaryActionId = 100});
    graph.AddNode(
        CoreEngine::DecisionNode{.id = "b", .type = CoreEngine::DecisionNodeType::Action, .primaryActionId = 200});

    std::string err;
    graph.AddConnection({"entry", 0, "pick", 0}, &err);
    graph.AddConnection({"pick", 0, "a", 0}, &err);
    graph.AddConnection({"pick", 1, "b", 0}, &err);

    MapDecisionSnapshot snapshot;
    CoreEngine::DecisionGraphInterpreter interpreter;

    DeterministicRNG rngA(777);
    DeterministicRNG rngB(777);
    const int first = interpreter.Evaluate(graph, snapshot, rngA).primaryActionId;
    const int second = interpreter.Evaluate(graph, snapshot, rngB).primaryActionId;
    EXPECT_EQ(first, second);
    EXPECT_TRUE(first == 100 || first == 200);
}

TEST(DecisionGraphTest, CycleHitsMaxStepsGuard) {
    CoreEngine::DecisionGraph graph;
    graph.AddNode(CoreEngine::DecisionNode{.id = "entry", .type = CoreEngine::DecisionNodeType::Entry});

    auto makePassThrough = [](const char* id) {
        CoreEngine::DecisionNode node;
        node.id = id;
        node.type = CoreEngine::DecisionNodeType::Condition;
        node.valueKind = CoreEngine::DecisionValueKind::Float;
        node.propertyKey = "meter";
        node.op = CoreEngine::ComparisonOp::GTE;
        node.floatThreshold = 0.0f;
        return node;
    };

    graph.AddNode(makePassThrough("hop_a"));
    graph.AddNode(makePassThrough("hop_b"));

    std::string err;
    ASSERT_TRUE(graph.AddConnection({"entry", 0, "hop_a", 0}, &err));
    ASSERT_TRUE(graph.AddConnection({"hop_a", 0, "hop_b", 0}, &err));
    ASSERT_TRUE(graph.AddConnection({"hop_b", 0, "hop_a", 0}, &err));

    MapDecisionSnapshot snapshot;
    DeterministicRNG rng(1);
    CoreEngine::DecisionGraphInterpreter interpreter(5, 42);
    const CoreEngine::DecisionEvalResult result = interpreter.Evaluate(graph, snapshot, rng);

    EXPECT_TRUE(result.defaultedOnGuard);
    EXPECT_EQ(result.primaryActionId, 42);
    EXPECT_GE(result.stepsExecuted, 5);
}

TEST(DecisionGraphTest, JsonRoundTrip) {
    CoreEngine::DecisionGraph graph = BuildEntryConditionActionGraph();

    CoreEngine::DecisionNode selector;
    selector.id = "cycle";
    selector.type = CoreEngine::DecisionNodeType::Selector;
    selector.selectorMode = CoreEngine::SelectorMode::Weighted;
    selector.weights = {0.25f, 1.75f};
    graph.AddNode(selector);

    const std::string json = graph.ToJsonString();
    CoreEngine::DecisionGraph parsed;
    std::string err;
    ASSERT_TRUE(CoreEngine::DecisionGraph::FromJsonString(json, &parsed, &err)) << err;

    EXPECT_EQ(parsed.Nodes().size(), graph.Nodes().size());
    EXPECT_EQ(parsed.Connections().size(), graph.Connections().size());

    const CoreEngine::DecisionNode* sel = parsed.FindNode("cycle");
    ASSERT_NE(sel, nullptr);
    ASSERT_EQ(sel->weights.size(), 2u);
    EXPECT_FLOAT_EQ(sel->weights[0], 0.25f);
    EXPECT_FLOAT_EQ(sel->weights[1], 1.75f);

    const std::string json2 = parsed.ToJsonString();
    CoreEngine::DecisionGraph parsed2;
    ASSERT_TRUE(CoreEngine::DecisionGraph::FromJsonString(json2, &parsed2, &err));
    EXPECT_EQ(parsed2.ToJsonString(), json2);
}

TEST(DecisionGraphTest, ValidateConnectionRejectsActionChainAndSelfLoop) {
    CoreEngine::DecisionGraph graph;
    graph.AddNode(
        CoreEngine::DecisionNode{.id = "a1", .type = CoreEngine::DecisionNodeType::Action, .primaryActionId = 1});
    graph.AddNode(
        CoreEngine::DecisionNode{.id = "a2", .type = CoreEngine::DecisionNodeType::Action, .primaryActionId = 2});
    graph.AddNode(CoreEngine::DecisionNode{.id = "entry", .type = CoreEngine::DecisionNodeType::Entry});

    std::string err;
    EXPECT_FALSE(graph.ValidateConnection("a1", 0, "a2", 0, &err));
    EXPECT_FALSE(graph.ValidateConnection("entry", 0, "entry", 0, &err));
}