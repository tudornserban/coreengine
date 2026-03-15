#include <gtest/gtest.h>
#include "core/gameloop/GameLogicEngine.h"

TEST(GameLogicEngineTest, DefaultsNotPaused) {
    GameLogicEngine engine;
    EXPECT_FALSE(engine.IsPaused());
}

TEST(GameLogicEngineTest, DefaultSpeedIsOne) {
    GameLogicEngine engine;
    EXPECT_FLOAT_EQ(engine.GetSpeedMultiplier(), 1.0f);
}

TEST(GameLogicEngineTest, GameTimeStartsAtZero) {
    GameLogicEngine engine;
    EXPECT_FLOAT_EQ(engine.GetGameTime(), 0.0f);
}

TEST(GameLogicEngineTest, CallbackInvokedOnUpdate) {
    GameLogicEngine engine(1.0f / 30.0f);
    int callCount = 0;
    engine.SetUpdateCallback([&](float /*dt*/) { ++callCount; });
    // Feed one fixed timestep worth of real time → should fire exactly once
    engine.Update(1.0f / 30.0f);
    EXPECT_GE(callCount, 1);
}

TEST(GameLogicEngineTest, CallbackNotCalledWhenPaused) {
    GameLogicEngine engine(1.0f / 30.0f);
    int callCount = 0;
    engine.SetUpdateCallback([&](float) { ++callCount; });
    engine.SetPaused(true);
    engine.Update(1.0f);
    EXPECT_EQ(callCount, 0);
}

TEST(GameLogicEngineTest, GameTimeAdvancesOnUpdate) {
    GameLogicEngine engine(1.0f / 30.0f);
    engine.SetUpdateCallback([](float) {});
    engine.Update(1.0f / 30.0f);
    EXPECT_GT(engine.GetGameTime(), 0.0f);
}

TEST(GameLogicEngineTest, ResetClearsGameTime) {
    GameLogicEngine engine(1.0f / 30.0f);
    engine.SetUpdateCallback([](float) {});
    engine.Update(1.0f);
    engine.Reset();
    EXPECT_FLOAT_EQ(engine.GetGameTime(), 0.0f);
}

TEST(GameLogicEngineTest, SetPausedWorks) {
    GameLogicEngine engine;
    engine.SetPaused(true);
    EXPECT_TRUE(engine.IsPaused());
    engine.SetPaused(false);
    EXPECT_FALSE(engine.IsPaused());
}

TEST(GameLogicEngineTest, SetSpeedMultiplierClampsAtZero) {
    GameLogicEngine engine;
    engine.SetSpeedMultiplier(-1.0f);
    EXPECT_FLOAT_EQ(engine.GetSpeedMultiplier(), 0.0f);
}

TEST(GameLogicEngineTest, FastForwardFiresCallbackManyTimes) {
    GameLogicEngine engine(1.0f / 30.0f);
    int count = 0;
    engine.SetUpdateCallback([&](float) { ++count; });
    engine.SetShouldEndCallback([&]() { return count >= 30; });
    engine.SetFastForward(true);
    engine.Update(0.016f);
    EXPECT_GE(count, 30);
}
