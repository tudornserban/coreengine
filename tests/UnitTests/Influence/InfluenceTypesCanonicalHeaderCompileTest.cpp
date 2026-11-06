#include <gtest/gtest.h>
#include "core/influence_types/InfluenceFieldItem.h"

TEST(CanonicalItemHeaderCompileTest, CreateAttractorUsesCanonicalHeaderOnly) {
    auto item = InfluenceFieldItem::CreateAttractor(Vector2(2.0f, 0.0f), 1.0f);
    EXPECT_FLOAT_EQ(item.GetPosition().x, 2.0f);
}