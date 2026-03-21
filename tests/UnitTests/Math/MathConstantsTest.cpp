#include <gtest/gtest.h>
#include "core/math/MathConstants.h"
#include <cmath>

using namespace CoreEngine::Math;

TEST(MathConstantsTest, PIIsCorrect) {
    EXPECT_NEAR(PI, 3.14159265358979f, 1e-6f);
}

TEST(MathConstantsTest, TwoPiIsDoublePi) {
    EXPECT_FLOAT_EQ(TWO_PI, 2.0f * PI);
}

TEST(MathConstantsTest, HalfPiIsHalfOfPi) {
    EXPECT_FLOAT_EQ(HALF_PI, PI * 0.5f);
}

TEST(MathConstantsTest, DegToRadConversion) {
    float rad = 180.0f * DEG_TO_RAD;
    EXPECT_NEAR(rad, PI, 1e-5f);
}

TEST(MathConstantsTest, RadToDegConversion) {
    float deg = PI * RAD_TO_DEG;
    EXPECT_NEAR(deg, 180.0f, 1e-3f);
}

TEST(MathConstantsTest, DegToRadAndBackIsIdentity) {
    float original = 45.0f;
    float roundtripped = original * DEG_TO_RAD * RAD_TO_DEG;
    EXPECT_NEAR(roundtripped, original, 1e-4f);
}

TEST(MathConstantsTest, EpsilonIsSmall) {
    EXPECT_LT(EPSILON, 1e-5f);
    EXPECT_GT(EPSILON, 0.0f);
}

TEST(MathConstantsTest, FloatMaxIsPositive) {
    EXPECT_GT(FLOAT_MAX, 0.0f);
}

TEST(MathConstantsTest, FloatMinIsPositive) {
    EXPECT_GT(FLOAT_MIN, 0.0f);
    EXPECT_LT(FLOAT_MIN, FLOAT_MAX);
}
