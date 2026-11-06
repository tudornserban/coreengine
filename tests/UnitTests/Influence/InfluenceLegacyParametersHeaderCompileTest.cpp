#include <gtest/gtest.h>
#include "core/influence/InfluenceFieldParameters.h"

TEST(LegacyParametersHeaderCompileTest, WrapperPathProvidesAttractorParams) {
    AttractorParams params;
    EXPECT_GT(params.AttractorStrength, 0.0f);
}