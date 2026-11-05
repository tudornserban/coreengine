#include <gtest/gtest.h>
#include "core/influence/InfluenceFieldItem.h"

TEST(LegacyItemHeaderCompileTest, WrapperPathProvidesInfluenceFieldItem) {
    auto item = InfluenceFieldItem::CreateRepulsor(Vector2(0.0f, 3.0f), 1.0f);
    EXPECT_EQ(item.GetInfluenceType(), InfluenceType::REPULSOR);
}
