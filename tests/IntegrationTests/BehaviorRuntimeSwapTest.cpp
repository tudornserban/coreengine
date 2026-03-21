/// Integration test for the "magic dust" mechanic from RationalInsanity:
/// a BehaviorFactory-registered behavior can be swapped into a running StateMachine
/// without restarting the engine or reconstructing any objects.

#include <gtest/gtest.h>
#include "core/ai/BehaviorFactory.h"
#include "core/ai/StateMachine.h"
#include "core/ai/BehaviorContext.h"
#include "core/ai/Behaviors/SeekBehavior.h"
#include "core/ai/Behaviors/FleeBehavior.h"
#include "tests/TestHelpers/MockBehavior.h"
#include "tests/TestHelpers/MockGameUnit.h"

class BehaviorRuntimeSwapTest : public ::testing::Test {
protected:
    void SetUp() override {
        BehaviorFactory::RegisterBuiltins();
        BehaviorFactory::Register("Patrol",
            [] { return std::make_unique<MockBehavior>("Patrol"); });
        BehaviorFactory::Register("Aggro",
            [] { return std::make_unique<MockBehavior>("Aggro"); });
    }
    void TearDown() override {
        BehaviorFactory::Unregister("Patrol");
        BehaviorFactory::Unregister("Aggro");
    }
};

TEST_F(BehaviorRuntimeSwapTest, SwappingBehaviorChangesAcceleration) {
    MockGameUnit unit;
    StateMachine sm;
    sm.SetOwner(&unit);

    // Patrol: returns acceleration (0, 0)
    auto patrol = std::make_unique<MockBehavior>("Patrol");
    patrol->SetAccelerationToReturn(Vector2(0.0f, 0.0f));

    // Aggro: returns acceleration (100, 0)
    auto aggro = std::make_unique<MockBehavior>("Aggro");
    aggro->SetAccelerationToReturn(Vector2(100.0f, 0.0f));

    sm.AddState("Patrol", std::move(patrol));
    sm.AddState("Aggro",  std::move(aggro));
    sm.SetState("Patrol");

    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;

    Vector2 before = sm.Update(ctx);
    EXPECT_FLOAT_EQ(before.x, 0.0f);

    // Fairy sprinkles magic dust — runtime swap
    sm.SetState("Aggro");

    Vector2 after = sm.Update(ctx);
    EXPECT_FLOAT_EQ(after.x, 100.0f);
}

TEST_F(BehaviorRuntimeSwapTest, FactoryProducesCorrectBehaviorType) {
    auto b = BehaviorFactory::Create("Seek");
    ASSERT_NE(b, nullptr);
    EXPECT_STREQ(b->GetName(), "Seek");

    b = BehaviorFactory::Create("Flee");
    ASSERT_NE(b, nullptr);
    EXPECT_STREQ(b->GetName(), "Flee");
}

TEST_F(BehaviorRuntimeSwapTest, RuntimeRegistrationAndInstantiation) {
    // Simulate a game registering its own behavior post-boot
    BehaviorFactory::Register("HealAura",
        [] { return std::make_unique<MockBehavior>("HealAura"); });

    ASSERT_TRUE(BehaviorFactory::IsRegistered("HealAura"));
    auto b = BehaviorFactory::Create("HealAura");
    ASSERT_NE(b, nullptr);
    EXPECT_STREQ(b->GetName(), "HealAura");

    BehaviorFactory::Unregister("HealAura");
    EXPECT_FALSE(BehaviorFactory::IsRegistered("HealAura"));
}

TEST_F(BehaviorRuntimeSwapTest, OnEnterOnExitFiredOnSwap) {
    MockGameUnit unit;
    StateMachine sm;
    sm.SetOwner(&unit);

    auto patrol = std::make_unique<MockBehavior>("Patrol");
    auto aggro  = std::make_unique<MockBehavior>("Aggro");
    MockBehavior* rawPatrol = patrol.get();
    MockBehavior* rawAggro  = aggro.get();

    sm.AddState("Patrol", std::move(patrol));
    sm.AddState("Aggro",  std::move(aggro));
    sm.SetState("Patrol");      // OnEnter(Patrol)

    sm.SetState("Aggro");       // OnExit(Patrol), OnEnter(Aggro)

    EXPECT_GE(rawPatrol->GetExitCount(),  1);
    EXPECT_GE(rawAggro->GetEnterCount(),  1);
    (void)rawPatrol;
    (void)rawAggro;
}
