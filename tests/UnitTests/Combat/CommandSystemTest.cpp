#include <gtest/gtest.h>

#include <type_traits>

#include "core/combat/CommandSystem.h"
#include "core/gamestate/RTSUnit.h"
#include "core/interfaces/IUnitAgent.h"
#include "core/math/Vector2.h"
#include "tests/TestHelpers/MockBehavior.h"
#include "tests/TestHelpers/MockGameUnit.h"

TEST(CommandSystemTest, MockGameUnit_NoIUnitAgent_SetStateAndParameterAreNoOp) {
    CommandSystem commands;
    MockGameUnit unit;

    static_assert(!std::is_base_of_v<IUnitAgent, MockGameUnit>);

    commands.SetUnitState(&unit, "any_state");
    commands.SetBehaviorParameter(&unit, "strength", 42.0f);
}

TEST(CommandSystemTest, RTSUnit_SetUnitStateDelegatesToStateMachine) {
    CommandSystem commands;
    RTSUnit unit(Vector2::Zero(), 0);

    unit.GetStateMachine().AddState("A", std::make_unique<MockBehavior>("BehA"));
    unit.GetStateMachine().AddState("B", std::make_unique<MockBehavior>("BehB"));

    commands.SetUnitState(&unit, "B");
    EXPECT_EQ(unit.GetStateMachine().GetCurrentState(), "B");
    ASSERT_NE(unit.GetStateMachine().GetCurrentBehavior(), nullptr);
    EXPECT_STREQ(unit.GetStateMachine().GetCurrentBehavior()->GetName(), "BehB");
}

TEST(CommandSystemTest, RTSUnit_SetBehaviorParameterUpdatesCurrentBehavior) {
    CommandSystem commands;
    RTSUnit unit(Vector2::Zero(), 0);

    auto beh = std::make_unique<MockBehavior>("Beh");
    beh->SetParameter("strength", 100.0f);
    unit.GetStateMachine().AddState("S", std::move(beh));
    unit.GetStateMachine().SetState("S");

    ASSERT_NE(unit.GetStateMachine().GetCurrentBehavior(), nullptr);
    EXPECT_FLOAT_EQ(unit.GetStateMachine().GetCurrentBehavior()->GetParameters().at("strength"), 100.0f);

    commands.SetBehaviorParameter(&unit, "strength", 7.5f);
    EXPECT_FLOAT_EQ(unit.GetStateMachine().GetCurrentBehavior()->GetParameters().at("strength"), 7.5f);
}
