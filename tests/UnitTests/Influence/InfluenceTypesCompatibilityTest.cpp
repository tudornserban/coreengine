#include <gtest/gtest.h>
#include "core/influence_types/InfluenceFieldItem.h"
#include "core/influence/InfluenceFieldItem.h"

TEST(InfluenceTypesCompatibilityTest, CanonicalAndWrapperHeadersCompileTogether) {
    auto item = InfluenceFieldItem::CreateRepulsor(Vector2(0.0f, 1.0f), 1.5f);
    EXPECT_EQ(item.GetInfluenceType(), InfluenceType::REPULSOR);
}

TEST(InfluenceTypesModuleTest, AggregateEmptyMatchesDefaultItem) {
    auto aggregated = InfluenceFieldItem::Aggregate({});
    EXPECT_FLOAT_EQ(aggregated.GetImportance(), 1.0f);
}
