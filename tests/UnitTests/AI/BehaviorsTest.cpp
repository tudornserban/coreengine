#include <gtest/gtest.h>
#include "core/ai/Behaviors/SeekBehavior.h"
#include "core/ai/Behaviors/FleeBehavior.h"
#include "core/ai/Behaviors/WanderBehavior.h"
#include "core/ai/Behaviors/SeparationBehavior.h"
#include "core/ai/Behaviors/CohesionBehavior.h"
#include "core/ai/BehaviorContext.h"
#include "core/math/DeterministicRNG.h"
#include "tests/TestHelpers/MockGameUnit.h"

// ============================================================
// SeekBehavior
// ============================================================

TEST(SeekBehaviorTest, ReturnsZeroWhenNoSelf) {
    SeekBehavior seek(Vector2(100.0f, 0.0f));
    BehaviorContext ctx;
    ctx.self = nullptr;
    ctx.deltaTime = 0.016f;
    EXPECT_FLOAT_EQ(seek.CalculateAcceleration(ctx).x, 0.0f);
    EXPECT_FLOAT_EQ(seek.CalculateAcceleration(ctx).y, 0.0f);
}

TEST(SeekBehaviorTest, AcceleratesInTargetDirection) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    SeekBehavior seek(Vector2(500.0f, 0.0f));
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    Vector2 a = seek.CalculateAcceleration(ctx);
    EXPECT_GT(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(SeekBehaviorTest, ReturnsZeroInsideArrivalRadius) {
    // Default arrivalRadius is 10; unit is 1 unit from target
    MockGameUnit unit(Vector2(499.0f, 0.0f));
    SeekBehavior seek(Vector2(500.0f, 0.0f));
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    Vector2 a = seek.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(SeekBehaviorTest, GetNameReturnsSeek) {
    SeekBehavior seek;
    EXPECT_STREQ(seek.GetName(), "Seek");
}

TEST(SeekBehaviorTest, SetAndGetTarget) {
    SeekBehavior seek;
    seek.SetTarget(Vector2(42.0f, 7.0f));
    EXPECT_FLOAT_EQ(seek.GetTarget().x, 42.0f);
    EXPECT_FLOAT_EQ(seek.GetTarget().y,  7.0f);
}

TEST(SeekBehaviorTest, AccelerationProportionalToDeltaTime) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    SeekBehavior s1(Vector2(500.0f, 0.0f));
    SeekBehavior s2(Vector2(500.0f, 0.0f));
    BehaviorContext c1; c1.self = &unit; c1.deltaTime = 0.016f;
    BehaviorContext c2; c2.self = &unit; c2.deltaTime = 0.032f;
    EXPECT_FLOAT_EQ(s2.CalculateAcceleration(c2).x, s1.CalculateAcceleration(c1).x * 2.0f);
}

// ============================================================
// FleeBehavior
// ============================================================

TEST(FleeBehaviorTest, ReturnsZeroWhenNoSelf) {
    FleeBehavior flee;
    BehaviorContext ctx;
    ctx.self = nullptr;
    ctx.deltaTime = 0.016f;
    EXPECT_FLOAT_EQ(flee.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(FleeBehaviorTest, ReturnsZeroWhenNoEnemies) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    FleeBehavior flee;
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    EXPECT_FLOAT_EQ(flee.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(FleeBehaviorTest, FleesAwayFromEnemy) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit enemy(Vector2(50.0f, 0.0f));
    FleeBehavior flee;
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyEnemies = {&enemy};
    ctx.deltaTime = 0.016f;
    EXPECT_LT(flee.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(FleeBehaviorTest, ReturnsZeroWhenEnemyBeyondDangerRadius) {
    // Default dangerRadius = 200; enemy at 300
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit enemy(Vector2(300.0f, 0.0f));
    FleeBehavior flee;
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyEnemies = {&enemy};
    ctx.deltaTime = 0.016f;
    Vector2 a = flee.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(FleeBehaviorTest, SymmetricEnemiesYieldZeroNetAcceleration) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit left(Vector2(-50.0f, 0.0f));
    MockGameUnit right(Vector2(50.0f, 0.0f));
    FleeBehavior flee;
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyEnemies = {&left, &right};
    ctx.deltaTime = 0.016f;
    Vector2 a = flee.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(FleeBehaviorTest, GetNameReturnsFlee) {
    EXPECT_STREQ(FleeBehavior{}.GetName(), "Flee");
}

// ============================================================
// WanderBehavior
// ============================================================

TEST(WanderBehaviorTest, ReturnsZeroWhenNoSelf) {
    DeterministicRNG rng(42);
    WanderBehavior wander;
    BehaviorContext ctx;
    ctx.self = nullptr;
    ctx.rng = &rng;
    ctx.deltaTime = 0.016f;
    EXPECT_FLOAT_EQ(wander.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(WanderBehaviorTest, ProducesNonZeroAcceleration) {
    DeterministicRNG rng(42);
    MockGameUnit unit(Vector2(500.0f, 500.0f));
    WanderBehavior wander;
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.rng = &rng;
    ctx.deltaTime = 0.016f;
    ctx.mapSize = Vector2(1000.0f, 1000.0f);
    EXPECT_GT(wander.CalculateAcceleration(ctx).Length(), 0.0f);
}

TEST(WanderBehaviorTest, IsDeterministicWithSameSeed) {
    MockGameUnit unit(Vector2(500.0f, 500.0f));
    auto run = [&](uint32_t seed) -> std::vector<float> {
        DeterministicRNG rng(seed);
        WanderBehavior wander;
        BehaviorContext ctx;
        ctx.self = &unit; ctx.rng = &rng;
        ctx.deltaTime = 0.1f; ctx.mapSize = Vector2(1000.0f, 1000.0f);
        std::vector<float> r;
        for (int i = 0; i < 30; ++i) {
            auto a = wander.CalculateAcceleration(ctx);
            r.push_back(a.x); r.push_back(a.y);
        }
        return r;
    };
    EXPECT_EQ(run(99), run(99));
}

TEST(WanderBehaviorTest, DifferentSeedsDifferentSequence) {
    MockGameUnit unit(Vector2(500.0f, 500.0f));
    auto run = [&](uint32_t seed) -> std::vector<float> {
        DeterministicRNG rng(seed);
        WanderBehavior wander;
        BehaviorContext ctx;
        ctx.self = &unit; ctx.rng = &rng;
        ctx.deltaTime = 0.1f; ctx.mapSize = Vector2(1000.0f, 1000.0f);
        std::vector<float> r;
        for (int i = 0; i < 30; ++i) {
            auto a = wander.CalculateAcceleration(ctx);
            r.push_back(a.x); r.push_back(a.y);
        }
        return r;
    };
    EXPECT_NE(run(1), run(2));
}

TEST(WanderBehaviorTest, GetNameReturnsWander) {
    EXPECT_STREQ(WanderBehavior{}.GetName(), "Wander");
}

// ============================================================
// SeparationBehavior
// ============================================================

TEST(SeparationBehaviorTest, ReturnsZeroWhenNoSelf) {
    SeparationBehavior sep;
    BehaviorContext ctx;
    ctx.self = nullptr;
    ctx.deltaTime = 0.016f;
    EXPECT_FLOAT_EQ(sep.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(SeparationBehaviorTest, ReturnsZeroWithNoAllies) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    SeparationBehavior sep;
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.deltaTime = 0.016f;
    EXPECT_FLOAT_EQ(sep.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(SeparationBehaviorTest, PushesAwayFromNearbyAlly) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit ally(Vector2(20.0f, 0.0f));
    SeparationBehavior sep;
    BehaviorContext ctx;
    ctx.self = &unit; ctx.nearbyAllies = {&unit, &ally}; ctx.deltaTime = 0.016f;
    EXPECT_LT(sep.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(SeparationBehaviorTest, ZeroWhenAllyOutsideSeparationRadius) {
    // Default separationRadius = 50
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit ally(Vector2(100.0f, 0.0f));
    SeparationBehavior sep;
    BehaviorContext ctx;
    ctx.self = &unit; ctx.nearbyAllies = {&unit, &ally}; ctx.deltaTime = 0.016f;
    Vector2 a = sep.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(SeparationBehaviorTest, IgnoresSelf) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    SeparationBehavior sep;
    BehaviorContext ctx;
    ctx.self = &unit; ctx.nearbyAllies = {&unit}; ctx.deltaTime = 0.016f;
    Vector2 a = sep.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(SeparationBehaviorTest, GetNameReturnsSeparation) {
    EXPECT_STREQ(SeparationBehavior{}.GetName(), "Separation");
}

// ============================================================
// CohesionBehavior
// ============================================================

TEST(CohesionBehaviorTest, ReturnsZeroWhenNoSelf) {
    CohesionBehavior coh;
    BehaviorContext ctx;
    ctx.self = nullptr;
    ctx.deltaTime = 0.016f;
    EXPECT_FLOAT_EQ(coh.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(CohesionBehaviorTest, ReturnsZeroWithNoAllies) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    CohesionBehavior coh;
    BehaviorContext ctx;
    ctx.self = &unit; ctx.deltaTime = 0.016f;
    EXPECT_FLOAT_EQ(coh.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(CohesionBehaviorTest, MovesTowardCenterOfAllies) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit ally(Vector2(100.0f, 0.0f));
    CohesionBehavior coh;
    BehaviorContext ctx;
    ctx.self = &unit; ctx.nearbyAllies = {&unit, &ally}; ctx.deltaTime = 0.016f;
    EXPECT_GT(coh.CalculateAcceleration(ctx).x, 0.0f);
}

TEST(CohesionBehaviorTest, ZeroWhenAllyBeyondPerceptionRadius) {
    // Default perceptionRadius = 150
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit ally(Vector2(200.0f, 0.0f));
    CohesionBehavior coh;
    BehaviorContext ctx;
    ctx.self = &unit; ctx.nearbyAllies = {&unit, &ally}; ctx.deltaTime = 0.016f;
    Vector2 a = coh.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(CohesionBehaviorTest, IgnoresSelf) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    CohesionBehavior coh;
    BehaviorContext ctx;
    ctx.self = &unit; ctx.nearbyAllies = {&unit}; ctx.deltaTime = 0.016f;
    Vector2 a = coh.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(CohesionBehaviorTest, SymmetricAlliesProduceZeroX) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit left(Vector2(-100.0f, 0.0f));
    MockGameUnit right(Vector2(100.0f, 0.0f));
    CohesionBehavior coh;
    BehaviorContext ctx;
    ctx.self = &unit; ctx.nearbyAllies = {&unit, &left, &right}; ctx.deltaTime = 0.016f;
    Vector2 a = coh.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(a.x, 0.0f);
    EXPECT_FLOAT_EQ(a.y, 0.0f);
}

TEST(CohesionBehaviorTest, GetNameReturnsCohesion) {
    EXPECT_STREQ(CohesionBehavior{}.GetName(), "Cohesion");
}
