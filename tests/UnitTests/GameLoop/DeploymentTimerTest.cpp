#include <gtest/gtest.h>
#include "core/gameloop/DeploymentTimer.h"

TEST(DeploymentTimerTest, StartsWithNoCharge) {
    DeploymentTimer t(1.0f);
    EXPECT_FALSE(t.HasCharge());
    EXPECT_EQ(t.GetCharges(), 0);
}

TEST(DeploymentTimerTest, ProgressStartsAtZero) {
    DeploymentTimer t(1.0f);
    EXPECT_FLOAT_EQ(t.GetChargeProgress(), 0.0f);
}

TEST(DeploymentTimerTest, ChargeAccumulatesOverTime) {
    DeploymentTimer t(1.0f);
    t.Update(0.5f);
    // Not yet full, but progress should be 0.5
    EXPECT_FLOAT_EQ(t.GetChargeProgress(), 0.5f);
    EXPECT_FALSE(t.HasCharge());
}

TEST(DeploymentTimerTest, ChargeReadyAfterFullTime) {
    DeploymentTimer t(1.0f);
    t.Update(1.0f);
    EXPECT_TRUE(t.HasCharge());
    EXPECT_EQ(t.GetCharges(), 1);
}

TEST(DeploymentTimerTest, SpendChargeDecrementsCount) {
    DeploymentTimer t(0.5f);
    t.Update(0.5f);
    ASSERT_TRUE(t.HasCharge());
    t.SpendCharge();
    EXPECT_FALSE(t.HasCharge());
    EXPECT_EQ(t.GetCharges(), 0);
}

TEST(DeploymentTimerTest, MultipleChargesAccumulate) {
    DeploymentTimer t(0.5f);
    t.Update(1.5f);   // 3 full charges
    EXPECT_GE(t.GetCharges(), 2);
}

TEST(DeploymentTimerTest, ProgressIsOneWhenChargeReady) {
    DeploymentTimer t(1.0f);
    t.Update(2.0f);   // definitely has a charge
    EXPECT_FLOAT_EQ(t.GetChargeProgress(), 1.0f);
}

TEST(DeploymentTimerTest, SpendChargeOnEmptyIsNoOp) {
    DeploymentTimer t(1.0f);
    EXPECT_NO_THROW(t.SpendCharge());
    EXPECT_EQ(t.GetCharges(), 0);
}
