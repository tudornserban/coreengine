#include <gtest/gtest.h>
#include "core/math/Vector2.h"
#include "tests/TestHelpers/TestHelpers.h"

// ============================================================
// Construction
// ============================================================

TEST(Vector2Test, DefaultConstructionIsZero) {
    Vector2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(Vector2Test, ParameterizedConstruction) {
    Vector2 v(5.0f, 10.0f);
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 10.0f);
}

// ============================================================
// Arithmetic operators
// ============================================================

TEST(Vector2Test, Addition) {
    Vector2 a(1.0f, 2.0f), b(3.0f, 4.0f);
    Vector2 r = a + b;
    EXPECT_FLOAT_EQ(r.x, 4.0f);
    EXPECT_FLOAT_EQ(r.y, 6.0f);
}

TEST(Vector2Test, Subtraction) {
    Vector2 a(3.0f, 4.0f), b(1.0f, 2.0f);
    Vector2 r = a - b;
    EXPECT_FLOAT_EQ(r.x, 2.0f);
    EXPECT_FLOAT_EQ(r.y, 2.0f);
}

TEST(Vector2Test, ScalarMultiplication) {
    Vector2 v(1.0f, 2.0f);
    Vector2 r = v * 2.0f;
    EXPECT_FLOAT_EQ(r.x, 2.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
}

TEST(Vector2Test, FriendScalarMultiplication) {
    Vector2 v(1.0f, 2.0f);
    Vector2 r = 2.0f * v;
    EXPECT_FLOAT_EQ(r.x, 2.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
}

TEST(Vector2Test, ScalarDivision) {
    Vector2 v(3.0f, 4.0f);
    Vector2 r = v / 2.0f;
    EXPECT_FLOAT_EQ(r.x, 1.5f);
    EXPECT_FLOAT_EQ(r.y, 2.0f);
}

TEST(Vector2Test, UnaryMinus) {
    Vector2 v(1.0f, -2.0f);
    Vector2 r = -v;
    EXPECT_FLOAT_EQ(r.x, -1.0f);
    EXPECT_FLOAT_EQ(r.y,  2.0f);
}

TEST(Vector2Test, CompoundAddAssign) {
    Vector2 v(1.0f, 2.0f);
    v += Vector2(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 6.0f);
}

TEST(Vector2Test, CompoundSubtractAssign) {
    Vector2 v(4.0f, 6.0f);
    v -= Vector2(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(v.x, 3.0f);
    EXPECT_FLOAT_EQ(v.y, 4.0f);
}

TEST(Vector2Test, CompoundMultiplyAssign) {
    Vector2 v(3.0f, 4.0f);
    v *= 2.0f;
    EXPECT_FLOAT_EQ(v.x, 6.0f);
    EXPECT_FLOAT_EQ(v.y, 8.0f);
}

// ============================================================
// Length / magnitude
// ============================================================

TEST(Vector2Test, Length) {
    EXPECT_FLOAT_EQ(Vector2(3.0f, 4.0f).Length(), 5.0f);
}

TEST(Vector2Test, LengthSquared) {
    EXPECT_FLOAT_EQ(Vector2(3.0f, 4.0f).LengthSquared(), 25.0f);
}

TEST(Vector2Test, LengthOfZeroVectorIsZero) {
    EXPECT_FLOAT_EQ(Vector2::Zero().Length(), 0.0f);
}

// ============================================================
// Normalization
// ============================================================

TEST(Vector2Test, NormalizedHasUnitLength) {
    Vector2 n = Vector2(3.0f, 4.0f).Normalized();
    EXPECT_FLOAT_EQ(n.Length(), 1.0f);
    EXPECT_FLOAT_EQ(n.x, 0.6f);
    EXPECT_FLOAT_EQ(n.y, 0.8f);
}

TEST(Vector2Test, NormalizeInPlace) {
    Vector2 v(3.0f, 4.0f);
    v.Normalize();
    EXPECT_FLOAT_EQ(v.Length(), 1.0f);
}

TEST(Vector2Test, NormalizedZeroVectorIsZero) {
    Vector2 n = Vector2::Zero().Normalized();
    EXPECT_FLOAT_EQ(n.Length(), 0.0f);
}

// ============================================================
// Dot product & distance
// ============================================================

TEST(Vector2Test, DotProduct) {
    EXPECT_FLOAT_EQ(Vector2(1.0f, 2.0f).Dot(Vector2(3.0f, 4.0f)), 11.0f);
}

TEST(Vector2Test, DotProductOrthogonalVectorsIsZero) {
    EXPECT_FLOAT_EQ(Vector2(1.0f, 0.0f).Dot(Vector2(0.0f, 1.0f)), 0.0f);
}

TEST(Vector2Test, Distance) {
    EXPECT_FLOAT_EQ(Vector2(0.0f, 0.0f).Distance(Vector2(3.0f, 4.0f)), 5.0f);
}

TEST(Vector2Test, DistanceSquared) {
    EXPECT_FLOAT_EQ(Vector2(0.0f, 0.0f).DistanceSquared(Vector2(3.0f, 4.0f)), 25.0f);
}

// ============================================================
// Static factories
// ============================================================

TEST(Vector2Test, StaticZero) {
    Vector2 z = Vector2::Zero();
    EXPECT_FLOAT_EQ(z.x, 0.0f);
    EXPECT_FLOAT_EQ(z.y, 0.0f);
}

TEST(Vector2Test, StaticOne) {
    Vector2 o = Vector2::One();
    EXPECT_FLOAT_EQ(o.x, 1.0f);
    EXPECT_FLOAT_EQ(o.y, 1.0f);
}

TEST(Vector2Test, StaticUnitX) {
    Vector2 u = Vector2::UnitX();
    EXPECT_FLOAT_EQ(u.x, 1.0f);
    EXPECT_FLOAT_EQ(u.y, 0.0f);
}

TEST(Vector2Test, StaticUnitY) {
    Vector2 u = Vector2::UnitY();
    EXPECT_FLOAT_EQ(u.x, 0.0f);
    EXPECT_FLOAT_EQ(u.y, 1.0f);
}

// ============================================================
// Lerp / Clamp / ClampMagnitude / ProjectOnto
// ============================================================

TEST(Vector2Test, LerpMidpoint) {
    Vector2 r = Vector2::Lerp(Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f), 0.5f);
    EXPECT_FLOAT_EQ(r.x, 5.0f);
    EXPECT_FLOAT_EQ(r.y, 5.0f);
}

TEST(Vector2Test, LerpAtZeroReturnsStart) {
    Vector2 a(1.0f, 2.0f), b(7.0f, 8.0f);
    EXPECT_TRUE(TestHelpers::Vector2Equals(Vector2::Lerp(a, b, 0.0f), a));
}

TEST(Vector2Test, LerpAtOneReturnsEnd) {
    Vector2 a(1.0f, 2.0f), b(7.0f, 8.0f);
    EXPECT_TRUE(TestHelpers::Vector2Equals(Vector2::Lerp(a, b, 1.0f), b));
}

TEST(Vector2Test, ClampComponentWise) {
    Vector2 clamped = Vector2::Clamp(Vector2(15.0f, -5.0f),
                                     Vector2(0.0f, 0.0f),
                                     Vector2(10.0f, 10.0f));
    EXPECT_FLOAT_EQ(clamped.x, 10.0f);
    EXPECT_FLOAT_EQ(clamped.y,  0.0f);
}

TEST(Vector2Test, ClampMagnitudeReducesOverlong) {
    Vector2 v(3.0f, 4.0f); // length 5
    Vector2 c = Vector2::ClampMagnitude(v, 2.5f);
    EXPECT_FLOAT_EQ(c.Length(), 2.5f);
    EXPECT_FLOAT_EQ(c.x, 1.5f);
    EXPECT_FLOAT_EQ(c.y, 2.0f);
}

TEST(Vector2Test, ClampMagnitudeKeepsShortVector) {
    Vector2 v(1.0f, 1.0f);
    Vector2 c = Vector2::ClampMagnitude(v, 5.0f);
    EXPECT_FLOAT_EQ(c.x, 1.0f);
    EXPECT_FLOAT_EQ(c.y, 1.0f);
}

TEST(Vector2Test, ProjectOntoUnitAxis) {
    Vector2 p = Vector2(3.0f, 4.0f).ProjectOnto(Vector2(1.0f, 0.0f));
    EXPECT_FLOAT_EQ(p.x, 3.0f);
    EXPECT_FLOAT_EQ(p.y, 0.0f);
}

TEST(Vector2Test, ProjectOntoZeroReturnsZero) {
    Vector2 p = Vector2(3.0f, 4.0f).ProjectOnto(Vector2::Zero());
    EXPECT_FLOAT_EQ(p.x, 0.0f);
    EXPECT_FLOAT_EQ(p.y, 0.0f);
}