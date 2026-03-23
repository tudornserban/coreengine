#include <gtest/gtest.h>
#include "core/ai/StateMachine.h"
#include "core/ai/BehaviorContext.h"
#include "core/ai/Conditions/TimeElapsedCondition.h"
#include "core/math/Vector2.h"
#include "tests/TestHelpers/MockBehavior.h"
#include "tests/TestHelpers/MockGameUnit.h"

TEST(StateMachineTest, AddState) {
    StateMachine sm;
    sm.AddState("A", std::make_unique<MockBehavior>("A"));
    EXPECT_EQ(sm.GetStates().size(), 1u);
    EXPECT_NE(sm.GetStates().find("A"), sm.GetStates().end());
}

TEST(StateMachineTest, SetStateChangesCurrentBehavior) {
    StateMachine sm;
    sm.AddState("A", std::make_unique<MockBehavior>("BehA"));
    sm.AddState("B", std::make_unique<MockBehavior>("BehB"));
    sm.SetState("A");
    ASSERT_NE(sm.GetCurrentBehavior(), nullptr);
    EXPECT_STREQ(sm.GetCurrentBehavior()->GetName(), "BehA");
    sm.SetState("B");
    EXPECT_STREQ(sm.GetCurrentBehavior()->GetName(), "BehB");
}

TEST(StateMachineTest, GetCurrentStateName) {
    StateMachine sm;
    sm.AddState("X", std::make_unique<MockBehavior>());
    sm.SetState("X");
    EXPECT_EQ(sm.GetCurrentState(), "X");
}

TEST(StateMachineTest, UpdateReturnsConfiguredAcceleration) {
    StateMachine sm;
    MockGameUnit unit;
    sm.SetOwner(&unit);

    auto beh = std::make_unique<MockBehavior>();
    beh->SetAccelerationToReturn(Vector2(10.0f, 20.0f));
    sm.AddState("S", std::move(beh));
    sm.SetState("S");

    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    Vector2 a = sm.Update(ctx);
    EXPECT_FLOAT_EQ(a.x, 10.0f);
    EXPECT_FLOAT_EQ(a.y, 20.0f);
}

TEST(StateMachineTest, AutomaticTransitionAfterTimeElapsed) {
    StateMachine sm;
    MockGameUnit unit;
    sm.SetOwner(&unit);
    sm.AddState("A", std::make_unique<MockBehavior>("A"));
    sm.AddState("B", std::make_unique<MockBehavior>("B"));
    sm.SetState("A");
    sm.AddTransition(StateTransition("A", "B",
        std::make_unique<TimeElapsedCondition>(1.0f), true));

    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    for (int i = 0; i < 100; ++i) sm.Update(ctx);
    EXPECT_EQ(sm.GetCurrentState(), "B");
}

TEST(StateMachineTest, TransitionDoesNotFireIfConditionNotMet) {
    StateMachine sm;
    MockGameUnit unit;
    sm.SetOwner(&unit);
    sm.AddState("A", std::make_unique<MockBehavior>());
    sm.AddState("B", std::make_unique<MockBehavior>());
    sm.SetState("A");
    sm.AddTransition(StateTransition("A", "B",
        std::make_unique<TimeElapsedCondition>(10.0f), true));

    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    for (int i = 0; i < 10; ++i) sm.Update(ctx);
    EXPECT_EQ(sm.GetCurrentState(), "A");
}

TEST(StateMachineTest, FirstSatisfiedTransitionWins) {
    StateMachine sm;
    MockGameUnit unit;
    sm.SetOwner(&unit);
    sm.AddState("A", std::make_unique<MockBehavior>());
    sm.AddState("B", std::make_unique<MockBehavior>());
    sm.AddState("C", std::make_unique<MockBehavior>());
    sm.SetState("A");
    // A→B fires at 1 s; A→C fires at 2 s — B should win
    sm.AddTransition(StateTransition("A", "B", std::make_unique<TimeElapsedCondition>(1.0f), true));
    sm.AddTransition(StateTransition("A", "C", std::make_unique<TimeElapsedCondition>(2.0f), true));

    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    for (int i = 0; i < 100; ++i) sm.Update(ctx);
    EXPECT_EQ(sm.GetCurrentState(), "B");
}

TEST(StateMachineTest, ClearCurrentState_ExitsBehaviorAndEmptiesCurrent) {
    StateMachine sm;
    MockGameUnit unit;
    sm.SetOwner(&unit);
    auto beh = std::make_unique<MockBehavior>("BehA");
    MockBehavior* raw = beh.get();
    sm.AddState("A", std::move(beh));
    sm.SetState("A");
    ASSERT_NE(sm.GetCurrentBehavior(), nullptr);

    sm.ClearCurrentState();
    EXPECT_TRUE(sm.GetCurrentState().empty());
    EXPECT_EQ(sm.GetCurrentBehavior(), nullptr);
    EXPECT_GE(raw->GetExitCount(), 1);
}

TEST(StateMachineTest, OnEnterOnExitCalledOnTransition) {
    StateMachine sm;
    MockGameUnit unit;
    sm.SetOwner(&unit);

    auto behA = std::make_unique<MockBehavior>("A");
    auto behB = std::make_unique<MockBehavior>("B");
    MockBehavior* rawA = behA.get();
    MockBehavior* rawB = behB.get();

    sm.AddState("A", std::move(behA));
    sm.AddState("B", std::move(behB));
    sm.SetState("A");
    sm.AddTransition(StateTransition("A", "B",
        std::make_unique<TimeElapsedCondition>(1.0f), true));

    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    for (int i = 0; i < 100; ++i) sm.Update(ctx);

    EXPECT_EQ(sm.GetCurrentState(), "B");
    EXPECT_GE(rawA->GetExitCount(),  1);
    EXPECT_GE(rawB->GetEnterCount(), 1);
}
