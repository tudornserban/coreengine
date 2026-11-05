#include <gtest/gtest.h>
#include "core/interfaces/IBehavior.h"
#include "core/ai/BehaviorContext.h"
#include <map>
#include <string>

namespace {

/// Minimal concrete behavior — exercises IBehavior's default GenerateFieldItems body.
class DefaultFieldItemsBehavior : public IBehavior {
public:
    const char* GetName() const override { return "DefaultFieldItemsBehavior"; }
    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

private:
    mutable std::map<std::string, float> params_;
};

} // namespace

// Regression: IBehavior.h must include complete InfluenceFieldItem (not forward-declare only).
// GCC 13 fails to compile std::vector<InfluenceFieldItem> in the interface otherwise.
TEST(IBehaviorInterfaceTest, DefaultGenerateFieldItemsReturnsEmptyVector) {
    DefaultFieldItemsBehavior behavior;
    BehaviorContext ctx;
    std::vector<InfluenceFieldItem> items = behavior.GenerateFieldItems(ctx);
    EXPECT_TRUE(items.empty());
}

TEST(IBehaviorInterfaceTest, DefaultCalculateAccelerationIsZero) {
    DefaultFieldItemsBehavior behavior;
    BehaviorContext ctx;
    Vector2 accel = behavior.CalculateAcceleration(ctx);
    EXPECT_FLOAT_EQ(accel.x, 0.0f);
    EXPECT_FLOAT_EQ(accel.y, 0.0f);
}
