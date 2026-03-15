#include <gtest/gtest.h>
#include "core/ai/Conditions/EnemyInRangeCondition.h"
#include "core/ai/Conditions/AlliesNearbyCondition.h"
#include "core/ai/Conditions/TimeElapsedCondition.h"
#include "core/ai/BehaviorContext.h"
#include "tests/TestHelpers/MockGameUnit.h"

// ============================================================
// EnemyInRangeCondition
// ============================================================

TEST(EnemyInRangeConditionTest, ReturnsFalseWhenNoSelf) {
    EnemyInRangeCondition cond(100.0f);
    BehaviorContext ctx;
    ctx.self = nullptr;
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(EnemyInRangeConditionTest, ReturnsFalseWhenNoEnemies) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    EnemyInRangeCondition cond(100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(EnemyInRangeConditionTest, ReturnsTrueWhenEnemyInRange) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit enemy(Vector2(50.0f, 0.0f));
    EnemyInRangeCondition cond(100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyEnemies = {&enemy};
    EXPECT_TRUE(cond.Evaluate(ctx));
}

TEST(EnemyInRangeConditionTest, ReturnsFalseWhenEnemyBeyondRange) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit enemy(Vector2(200.0f, 0.0f));
    EnemyInRangeCondition cond(100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyEnemies = {&enemy};
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(EnemyInRangeConditionTest, ReturnsTrueWhenEnemyExactlyAtRange) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit enemy(Vector2(100.0f, 0.0f));
    EnemyInRangeCondition cond(100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyEnemies = {&enemy};
    EXPECT_TRUE(cond.Evaluate(ctx));
}

TEST(EnemyInRangeConditionTest, ReturnsTrueWhenAnyEnemyInRange) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit far(Vector2(500.0f, 0.0f));
    MockGameUnit near(Vector2(30.0f, 0.0f));
    EnemyInRangeCondition cond(100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyEnemies = {&far, &near};
    EXPECT_TRUE(cond.Evaluate(ctx));
}

TEST(EnemyInRangeConditionTest, SetRangeAndGetRange) {
    EnemyInRangeCondition cond(50.0f);
    EXPECT_FLOAT_EQ(cond.GetRange(), 50.0f);
    cond.SetRange(200.0f);
    EXPECT_FLOAT_EQ(cond.GetRange(), 200.0f);
}

TEST(EnemyInRangeConditionTest, GetDescriptionIsNotNull) {
    EXPECT_NE(EnemyInRangeCondition(75.0f).GetDescription(), nullptr);
}

// ============================================================
// AlliesNearbyCondition
// ============================================================

TEST(AlliesNearbyConditionTest, ReturnsFalseWhenNoSelf) {
    AlliesNearbyCondition cond(1, 100.0f);
    BehaviorContext ctx;
    ctx.self = nullptr;
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(AlliesNearbyConditionTest, ReturnsFalseWhenNoAllies) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    AlliesNearbyCondition cond(1, 100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(AlliesNearbyConditionTest, ReturnsTrueWhenEnoughAlliesInRange) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit a1(Vector2(50.0f, 0.0f));
    MockGameUnit a2(Vector2(0.0f, 50.0f));
    AlliesNearbyCondition cond(2, 100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyAllies = {&unit, &a1, &a2};
    EXPECT_TRUE(cond.Evaluate(ctx));
}

TEST(AlliesNearbyConditionTest, ReturnsFalseWhenBelowThreshold) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit ally(Vector2(50.0f, 0.0f));
    AlliesNearbyCondition cond(2, 100.0f);   // needs 2, only 1 non-self
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyAllies = {&unit, &ally};
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(AlliesNearbyConditionTest, ReturnsFalseWhenAlliesOutsideRange) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit ally(Vector2(500.0f, 0.0f));
    AlliesNearbyCondition cond(1, 100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyAllies = {&unit, &ally};
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(AlliesNearbyConditionTest, DoesNotCountSelf) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    AlliesNearbyCondition cond(1, 100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyAllies = {&unit};
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(AlliesNearbyConditionTest, AllyAtExactRangeBoundaryCounts) {
    MockGameUnit unit(Vector2(0.0f, 0.0f));
    MockGameUnit ally(Vector2(100.0f, 0.0f));
    AlliesNearbyCondition cond(1, 100.0f);
    BehaviorContext ctx;
    ctx.self = &unit;
    ctx.nearbyAllies = {&unit, &ally};
    EXPECT_TRUE(cond.Evaluate(ctx));
}

TEST(AlliesNearbyConditionTest, GetDescriptionIsNotNull) {
    EXPECT_NE(AlliesNearbyCondition(2, 150.0f).GetDescription(), nullptr);
}

// ============================================================
// TimeElapsedCondition
// ============================================================

TEST(TimeElapsedConditionTest, ReturnsFalseBeforeThreshold) {
    TimeElapsedCondition cond(1.0f);
    BehaviorContext ctx;
    ctx.deltaTime = 0.016f;
    for (int i = 0; i < 10; ++i) cond.Evaluate(ctx);  // 0.16 s total
    EXPECT_FALSE(cond.Evaluate(ctx));
}

TEST(TimeElapsedConditionTest, ReturnsTrueAfterThreshold) {
    TimeElapsedCondition cond(0.5f);
    BehaviorContext ctx;
    ctx.deltaTime = 0.1f;
    bool triggered = false;
    for (int i = 0; i < 7; ++i) {
        if (cond.Evaluate(ctx)) { triggered = true; break; }
    }
    EXPECT_TRUE(triggered);
}

TEST(TimeElapsedConditionTest, ResetRestartsTimer) {
    TimeElapsedCondition cond(0.2f);
    BehaviorContext ctx;
    ctx.deltaTime = 0.1f;
    // Trigger once
    bool first = false;
    for (int i = 0; i < 5; ++i) {
        if (cond.Evaluate(ctx)) { first = true; break; }
    }
    EXPECT_TRUE(first);
    // Should require another 0.2 s worth of ticks
    EXPECT_FALSE(cond.Evaluate(ctx));
}
