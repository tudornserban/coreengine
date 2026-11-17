#include <gtest/gtest.h>

#include "core/math/ArenaGeometry.h"

TEST(ArenaGeometryTest, ClampedDeltaIgnoresArenaSize) {
    const Vector2 delta = ArenaGeometry::Delta(0.0f, 0.0f, 10.0f, -4.0f, 12.0f, 12.0f, false);
    EXPECT_FLOAT_EQ(delta.x, 10.0f);
    EXPECT_FLOAT_EQ(delta.y, -4.0f);
    EXPECT_FLOAT_EQ(ArenaGeometry::Distance(0.0f, 0.0f, 3.0f, 4.0f, 100.0f, 100.0f, false), 5.0f);
}

TEST(ArenaGeometryTest, WrappedDeltaTakesTheShortestSeam) {
    const Vector2 delta = ArenaGeometry::Delta(1.0f, 2.0f, 11.0f, 2.0f, 12.0f, 20.0f, true);
    EXPECT_FLOAT_EQ(delta.x, -2.0f);
    EXPECT_FLOAT_EQ(delta.y, 0.0f);
    EXPECT_FLOAT_EQ(ArenaGeometry::DistanceSq(1.0f, 2.0f, 11.0f, 2.0f, 12.0f, 20.0f, true), 4.0f);
}

TEST(ArenaGeometryTest, WrapPositionFoldsNegativeAndOverflow) {
    const Vector2 wrapped = ArenaGeometry::WrapPosition(-1.0f, 25.5f, 10.0f, 10.0f);
    EXPECT_FLOAT_EQ(wrapped.x, 9.0f);
    EXPECT_FLOAT_EQ(wrapped.y, 5.5f);
    const Vector2 edge = ArenaGeometry::WrapPosition(10.0f, 0.0f, 10.0f, 8.0f);
    EXPECT_FLOAT_EQ(edge.x, 0.0f);
    EXPECT_FLOAT_EQ(edge.y, 0.0f);
}

TEST(ArenaGeometryTest, ClampPositionStaysInsideTheArena) {
    const Vector2 clamped = ArenaGeometry::ClampPosition(-4.0f, 30.0f, 12.0f, 9.0f);
    EXPECT_FLOAT_EQ(clamped.x, 0.0f);
    EXPECT_FLOAT_EQ(clamped.y, 9.0f);
}
