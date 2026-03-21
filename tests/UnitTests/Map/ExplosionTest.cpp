#include <gtest/gtest.h>
#include "core/combat/Explosion.h"
#include "core/math/Vector2.h"
#include "tests/TestHelpers/TestHelpers.h"

// ============================================================
// Construction / getters
// ============================================================

TEST(ExplosionTest, ConstructionStoresValues) {
    Explosion e(Vector2(100.0f, 200.0f), 50.0f, 100.0f, 2.0f);
    EXPECT_TRUE(TestHelpers::Vector2Equals(e.GetPosition(), Vector2(100.0f, 200.0f)));
    EXPECT_FLOAT_EQ(e.GetRadius(), 50.0f);
    EXPECT_FLOAT_EQ(e.GetStrength(), 100.0f);
    EXPECT_FLOAT_EQ(e.GetFalloffExponent(), 2.0f);
}

TEST(ExplosionTest, DefaultStrengthIsZero) {
    Explosion e(Vector2(0.0f, 0.0f), 50.0f);
    EXPECT_FLOAT_EQ(e.GetStrength(), Explosion::DEFAULT_EXPLOSION_STRENGTH);
    EXPECT_FLOAT_EQ(e.GetStrength(), 0.0f);
}

TEST(ExplosionTest, DefaultFalloffIsLinear) {
    Explosion e(Vector2(0.0f, 0.0f), 50.0f, 100.0f);
    EXPECT_FLOAT_EQ(e.GetFalloffExponent(), 1.0f);
}

// ============================================================
// IsInRadius
// ============================================================

TEST(ExplosionTest, CenterIsInRadius) {
    Explosion e(Vector2(100.0f, 100.0f), 50.0f, 100.0f);
    EXPECT_TRUE(e.IsInRadius(Vector2(100.0f, 100.0f)));
}

TEST(ExplosionTest, PositionInsideIsInRadius) {
    Explosion e(Vector2(100.0f, 100.0f), 50.0f, 100.0f);
    EXPECT_TRUE(e.IsInRadius(Vector2(120.0f, 100.0f)));  // 20 < 50
}

TEST(ExplosionTest, PositionAtEdgeIsInRadius) {
    Explosion e(Vector2(100.0f, 100.0f), 50.0f, 100.0f);
    EXPECT_TRUE(e.IsInRadius(Vector2(150.0f, 100.0f)));  // exactly at radius
}

TEST(ExplosionTest, PositionOutsideIsNotInRadius) {
    Explosion e(Vector2(100.0f, 100.0f), 50.0f, 100.0f);
    EXPECT_FALSE(e.IsInRadius(Vector2(160.0f, 100.0f)));  // 60 > 50
}

// ============================================================
// CalculateImpulse
// ============================================================

TEST(ExplosionTest, ZeroStrengthGivesZeroImpulse) {
    Explosion e(Vector2(0.0f, 0.0f), 50.0f, 0.0f);
    EXPECT_TRUE(TestHelpers::Vector2Equals(
        e.CalculateImpulse(Vector2(10.0f, 0.0f), 100.0f), Vector2::Zero()));
}

TEST(ExplosionTest, UnitAtExactCenterGivesZeroImpulse) {
    Explosion e(Vector2(0.0f, 0.0f), 50.0f, 100.0f);
    EXPECT_TRUE(TestHelpers::Vector2Equals(
        e.CalculateImpulse(Vector2(0.0f, 0.0f), 100.0f), Vector2::Zero()));
}

TEST(ExplosionTest, UnitOutsideRadiusGivesZeroImpulse) {
    Explosion e(Vector2(0.0f, 0.0f), 50.0f, 100.0f);
    EXPECT_TRUE(TestHelpers::Vector2Equals(
        e.CalculateImpulse(Vector2(100.0f, 0.0f), 100.0f), Vector2::Zero()));
}

TEST(ExplosionTest, ImpulseDirectedAwayFromCenter) {
    Explosion e(Vector2(0.0f, 0.0f), 50.0f, 100.0f);
    Vector2 impulse = e.CalculateImpulse(Vector2(20.0f, 0.0f), 100.0f);
    EXPECT_GT(impulse.x, 0.0f);
    EXPECT_NEAR(impulse.y, 0.0f, 0.01f);
}

TEST(ExplosionTest, ImpulseDecreasesWithDistance) {
    Explosion e(Vector2(0.0f, 0.0f), 100.0f, 100.0f, 1.0f);
    float nearMag = e.CalculateImpulse(Vector2(25.0f, 0.0f), 100.0f).Length();
    float farMag  = e.CalculateImpulse(Vector2(75.0f, 0.0f), 100.0f).Length();
    EXPECT_GT(nearMag, farMag);
}

TEST(ExplosionTest, LighterUnitReceivesMoreImpulse) {
    Explosion e(Vector2(0.0f, 0.0f), 50.0f, 100.0f);
    float lightImpulse = e.CalculateImpulse(Vector2(10.0f, 0.0f), 50.0f).Length();
    float heavyImpulse = e.CalculateImpulse(Vector2(10.0f, 0.0f), 200.0f).Length();
    EXPECT_GT(lightImpulse, heavyImpulse);
}

TEST(ExplosionTest, QuadraticFalloffHasSmallerMidpointImpulse) {
    // At 50% radius, quadratic gives (1 - 0.5)^2 = 0.25, linear gives 0.5
    Explosion linear(Vector2(0.0f, 0.0f), 100.0f, 100.0f, 1.0f);
    Explosion quadratic(Vector2(0.0f, 0.0f), 100.0f, 100.0f, 2.0f);
    float lMag = linear.CalculateImpulse(Vector2(50.0f, 0.0f), 100.0f).Length();
    float qMag = quadratic.CalculateImpulse(Vector2(50.0f, 0.0f), 100.0f).Length();
    EXPECT_GT(lMag, qMag);
}
