#include <gtest/gtest.h>
#include "core/influence_types/InfluenceFieldItem.h"

TEST(InfluenceTypesModuleTest, AggregateEmptyMatchesDefaultItem) {
    auto aggregated = InfluenceFieldItem::Aggregate({});
    EXPECT_FLOAT_EQ(aggregated.GetImportance(), 1.0f);
}
