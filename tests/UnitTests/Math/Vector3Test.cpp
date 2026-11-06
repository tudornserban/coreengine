#include <gtest/gtest.h>
#include "core/math/Vector3.h"

TEST(Vector3Test, DefaultConstructionIsZero) {
    Vector3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vector3Test, LengthAndNormalization) {
    Vector3 v(0.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.Length(), 5.0f);
    Vector3 n = v.Normalized();
    EXPECT_FLOAT_EQ(n.Length(), 1.0f);
}

TEST(Vector3Test, CrossProductRightHanded) {
    Vector3 x = Vector3::UnitX();
    Vector3 y = Vector3::UnitY();
    Vector3 z = x.Cross(y);
    EXPECT_FLOAT_EQ(z.x, 0.0f);
    EXPECT_FLOAT_EQ(z.y, 0.0f);
    EXPECT_FLOAT_EQ(z.z, 1.0f);
}

TEST(Vector3Test, DotOrthogonalIsZero) {
    EXPECT_FLOAT_EQ(Vector3::UnitX().Dot(Vector3::UnitY()), 0.0f);
}

TEST(Vector3Test, ClampMagnitudeCapsLength) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    Vector3 c = Vector3::ClampMagnitude(v, 2.0f);
    EXPECT_FLOAT_EQ(c.Length(), 2.0f);
}

TEST(Vector3Test, ScalarMultiplyFriend) {
    Vector3 v(1.0f, 2.0f, 3.0f);
    Vector3 r = 2.0f * v;
    EXPECT_FLOAT_EQ(r.x, 2.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
    EXPECT_FLOAT_EQ(r.z, 6.0f);
}