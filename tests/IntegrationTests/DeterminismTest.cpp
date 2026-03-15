/// Integration test: running the game loop with a deterministic RNG seed must
/// produce an identical unit position sequence across two independent runs.
///
/// This validates the core invariant required for replays and multiplayer sync.

#include <gtest/gtest.h>
#include "core/math/DeterministicRNG.h"
#include "core/ai/BehaviorContext.h"
#include "core/ai/Behaviors/WanderBehavior.h"
#include "core/ai/Behaviors/SeekBehavior.h"
#include "core/ai/StateMachine.h"
#include "core/gameloop/GameLogicEngine.h"
#include "tests/TestHelpers/MockGameUnit.h"
#include <vector>

static std::vector<float> RunDeterministicSimulation(uint32_t seed, int steps) {
    DeterministicRNG rng(seed);
    MockGameUnit unit(Vector2(500.0f, 500.0f));
    WanderBehavior wander;
    wander.GetParameters()["directionChangeTime"] = 0.5f;  // change direction every 15 frames so RNG is exercised
    SeekBehavior   seek(Vector2(800.0f, 800.0f));

    BehaviorContext ctx;
    ctx.self     = &unit;
    ctx.rng      = &rng;
    ctx.deltaTime = 1.0f / 30.0f;
    ctx.mapSize   = Vector2(1000.0f, 1000.0f);

    std::vector<float> positions;
    for (int i = 0; i < steps; ++i) {
        // Alternate between wander and seek to exercise both paths
        IBehavior* beh = (i % 2 == 0) ? static_cast<IBehavior*>(&wander)
                                       : static_cast<IBehavior*>(&seek);
        Vector2 accel = beh->CalculateAcceleration(ctx);
        unit.ApplyAcceleration(accel, ctx.deltaTime);
        unit.Update(ctx.deltaTime);
        positions.push_back(unit.GetPosition().x);
        positions.push_back(unit.GetPosition().y);
    }
    return positions;
}

TEST(DeterminismTest, SameSeedProducesIdenticalPositionSequence) {
    auto run1 = RunDeterministicSimulation(42, 60);
    auto run2 = RunDeterministicSimulation(42, 60);
    EXPECT_EQ(run1, run2);
}

TEST(DeterminismTest, DifferentSeedsProduceDifferentPositions) {
    auto run1 = RunDeterministicSimulation(1,  60);
    auto run2 = RunDeterministicSimulation(99, 60);
    EXPECT_NE(run1, run2);
}

TEST(DeterminismTest, GameLogicEngineFixedTimestepIsConsistent) {
    float ts = 1.0f / 30.0f;
    GameLogicEngine engine(ts);

    int    callCount = 0;
    float  totalTime = 0.0f;

    engine.SetUpdateCallback([&](float dt) {
        ++callCount;
        totalTime += dt;
    });

    // Simulate 1 second of wall-clock time in 10ms chunks
    for (int i = 0; i < 100; ++i)
        engine.Update(0.01f);

    // Should have fired approximately 30 times (1s / (1/30)s)
    EXPECT_NEAR(callCount, 30, 2);
    // Each tick dt should equal the fixed timestep
    EXPECT_NEAR(totalTime, callCount * ts, 1e-4f);
}
