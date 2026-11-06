#include <gtest/gtest.h>

#include "core/gameloop/DeploymentTimer.h"
#include "core/gameloop/ThinkTimer.h"

namespace {

/// Locks behavior that is implied by the API but not spelled out in headers:
/// inactive think banks do not drain; deployment fill fraction stays in [0, 1].
class GameloopInvariantsTest : public ::testing::Test {};

}  // namespace

TEST_F(GameloopInvariantsTest, ThinkTimer_InactiveUpdateDoesNotDrainBank) {
    ThinkTimer timer(10.0f);
    ASSERT_FALSE(timer.IsActive());
    const float before = timer.GetBankRemaining();
    timer.Update(5.0f);
    EXPECT_FLOAT_EQ(timer.GetBankRemaining(), before);
    EXPECT_FALSE(timer.IsExhausted());
}

TEST_F(GameloopInvariantsTest, DeploymentTimer_ChargeProgressStaysInClosedUnitInterval) {
    DeploymentTimer timer(2.0f);

    timer.Update(1.0f);
    ASSERT_FALSE(timer.HasCharge());
    const float mid = timer.GetChargeProgress();
    EXPECT_GE(mid, 0.0f);
    EXPECT_LE(mid, 1.0f);

    timer.Update(1.0f);
    ASSERT_TRUE(timer.HasCharge());
    EXPECT_FLOAT_EQ(timer.GetChargeProgress(), 1.0f);

    timer.SpendCharge();
    EXPECT_FALSE(timer.HasCharge());
    const float afterSpend = timer.GetChargeProgress();
    EXPECT_GE(afterSpend, 0.0f);
    EXPECT_LE(afterSpend, 1.0f);
}