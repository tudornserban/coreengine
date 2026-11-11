#include <gtest/gtest.h>

#include <type_traits>
#include <vector>

#include "core/combat/CommandSystem.h"
#include "core/interfaces/IGameUnit.h"
#include "core/interfaces/IUnitAgent.h"
#include "core/math/Vector2.h"
#include "tests/TestHelpers/MockGameUnit.h"
#include "tests/TestHelpers/TestHelpers.h"

namespace {

class AgentRecordingUnit : public MockGameUnit, public IUnitAgent {
public:
    std::vector<std::string> recordedStates;
    struct RecordedParameter {
        std::string name;
        float value;
    };
    std::vector<RecordedParameter> recordedParameters;

    void SetAgentState(const std::string& stateName) override { recordedStates.push_back(stateName); }

    void SetAgentParameter(const std::string& paramName, float value) override {
        recordedParameters.push_back({paramName, value});
    }
};

class DebugDestinationRecordingUnit : public MockGameUnit {
public:
    Vector2 lastDebugDestination{Vector2::Zero()};

    void SetDebugDestination(Vector2 target) override {
        lastDebugDestination = target;
        MockGameUnit::SetDebugDestination(target);
    }
};

}  // namespace

TEST(CommandSystemAgentContractTest, ForwardsSetUnitStateToIUnitAgent) {
    CommandSystem commands;
    AgentRecordingUnit unit;

    static_assert(std::is_base_of_v<IUnitAgent, AgentRecordingUnit>);

    commands.SetUnitState(&unit, "patrol");
    ASSERT_EQ(unit.recordedStates.size(), 1u);
    EXPECT_EQ(unit.recordedStates.front(), "patrol");
}

TEST(CommandSystemAgentContractTest, ForwardsSetUnitsStateToEachIUnitAgent) {
    CommandSystem commands;
    AgentRecordingUnit first;
    AgentRecordingUnit second;

    commands.SetUnitsState({&first, &second}, "engage");

    ASSERT_EQ(first.recordedStates.size(), 1u);
    ASSERT_EQ(second.recordedStates.size(), 1u);
    EXPECT_EQ(first.recordedStates.front(), "engage");
    EXPECT_EQ(second.recordedStates.front(), "engage");
}

TEST(CommandSystemAgentContractTest, ForwardsSetBehaviorParameterToIUnitAgent) {
    CommandSystem commands;
    AgentRecordingUnit unit;

    commands.SetBehaviorParameter(&unit, "strength", 12.5f);
    ASSERT_EQ(unit.recordedParameters.size(), 1u);
    EXPECT_EQ(unit.recordedParameters.front().name, "strength");
    EXPECT_FLOAT_EQ(unit.recordedParameters.front().value, 12.5f);
}

TEST(CommandSystemAgentContractTest, MockGameUnitWithoutIUnitAgentIsNoOp) {
    CommandSystem commands;
    MockGameUnit unit;

    static_assert(!std::is_base_of_v<IUnitAgent, MockGameUnit>);

    commands.SetUnitState(&unit, "ignored");
    commands.SetBehaviorParameter(&unit, "ignored", 1.0f);
}

TEST(CommandSystemAgentContractTest, MoveUnitsToSetsDebugDestinationOnUnit) {
    CommandSystem commands;
    DebugDestinationRecordingUnit unit;
    const Vector2 destination(400.0f, 300.0f);

    commands.MoveUnitsTo({&unit}, destination);

    EXPECT_TRUE(TestHelpers::Vector2Equals(unit.lastDebugDestination, destination));
}
