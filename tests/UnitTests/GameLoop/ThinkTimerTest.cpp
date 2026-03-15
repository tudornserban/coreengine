#include <gtest/gtest.h>
#include "core/gameloop/ThinkTimer.h"

TEST(ThinkTimerTest, StartsInactive) {
    ThinkTimer t(5.0f);
    EXPECT_FALSE(t.IsActive());
}

TEST(ThinkTimerTest, FullBankOnConstruction) {
    ThinkTimer t(5.0f);
    EXPECT_FLOAT_EQ(t.GetBankRemaining(), 5.0f);
}

TEST(ThinkTimerTest, NotExhaustedOnConstruction) {
    ThinkTimer t(5.0f);
    EXPECT_FALSE(t.IsExhausted());
}

TEST(ThinkTimerTest, ActivateStartsDraining) {
    ThinkTimer t(5.0f);
    t.Activate();
    EXPECT_TRUE(t.IsActive());
    t.Update(1.0f);
    EXPECT_FLOAT_EQ(t.GetBankRemaining(), 4.0f);
}

TEST(ThinkTimerTest, InactiveDoesNotDrain) {
    ThinkTimer t(5.0f);
    t.Update(2.0f);  // not activated
    EXPECT_FLOAT_EQ(t.GetBankRemaining(), 5.0f);
}

TEST(ThinkTimerTest, DeactivateStopsDraining) {
    ThinkTimer t(5.0f);
    t.Activate();
    t.Update(1.0f);
    t.Deactivate();
    t.Update(1.0f);  // should not drain
    EXPECT_FLOAT_EQ(t.GetBankRemaining(), 4.0f);
}

TEST(ThinkTimerTest, AutoDeactivatesOnExhaustion) {
    ThinkTimer t(1.0f);
    t.Activate();
    t.Update(2.0f);  // drain more than available
    EXPECT_TRUE(t.IsExhausted());
    EXPECT_FALSE(t.IsActive());
}

TEST(ThinkTimerTest, ActivateOnExhaustedBankIsNoOp) {
    ThinkTimer t(0.5f);
    t.Activate();
    t.Update(1.0f);
    ASSERT_TRUE(t.IsExhausted());
    t.Activate();  // should not re-activate
    EXPECT_FALSE(t.IsActive());
}

TEST(ThinkTimerTest, DoubleActivateIsNoOp) {
    ThinkTimer t(5.0f);
    t.Activate();
    t.Activate();  // second call should be harmless
    t.Update(1.0f);
    EXPECT_FLOAT_EQ(t.GetBankRemaining(), 4.0f);
}
