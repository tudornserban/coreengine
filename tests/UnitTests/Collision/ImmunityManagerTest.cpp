#include <gtest/gtest.h>
#include "core/collision/ImmunityManager.h"
#include "tests/TestHelpers/MockGameUnit.h"

// ============================================================
// Construction / accessors
// ============================================================

TEST(ImmunityManagerTest, DefaultFramesAccessors) {
    ImmunityManager mgr(5);
    EXPECT_EQ(mgr.GetDefaultImmunityFrames(), 5);
    mgr.SetDefaultImmunityFrames(10);
    EXPECT_EQ(mgr.GetDefaultImmunityFrames(), 10);
}

// ============================================================
// ShouldGenerateEvent
// ============================================================

TEST(ImmunityManagerTest, AllowsEventByDefault) {
    ImmunityManager mgr;
    MockGameUnit a, b;
    EXPECT_TRUE(mgr.ShouldGenerateEvent(&a, &b));
}

TEST(ImmunityManagerTest, SuppressesEventAfterSetImmunity) {
    ImmunityManager mgr;
    MockGameUnit a, b;
    mgr.SetImmunity(&a, &b, 5);
    EXPECT_FALSE(mgr.ShouldGenerateEvent(&a, &b));
}

TEST(ImmunityManagerTest, ImmunityIsReciprocal) {
    ImmunityManager mgr;
    MockGameUnit a, b;
    mgr.SetImmunity(&a, &b, 5);
    EXPECT_FALSE(mgr.ShouldGenerateEvent(&a, &b));
    EXPECT_FALSE(mgr.ShouldGenerateEvent(&b, &a));
}

TEST(ImmunityManagerTest, NullArgumentsAlwaysAllow) {
    ImmunityManager mgr;
    MockGameUnit u;
    EXPECT_TRUE(mgr.ShouldGenerateEvent(nullptr, &u));
    EXPECT_TRUE(mgr.ShouldGenerateEvent(&u, nullptr));
    EXPECT_TRUE(mgr.ShouldGenerateEvent(nullptr, nullptr));
}

TEST(ImmunityManagerTest, SelfPairingIsNoOp) {
    ImmunityManager mgr;
    MockGameUnit u;
    EXPECT_NO_THROW(mgr.SetImmunity(&u, &u, 5));
    EXPECT_TRUE(mgr.ShouldGenerateEvent(&u, &u));
}

TEST(ImmunityManagerTest, ZeroFramesHasNoEffect) {
    ImmunityManager mgr;
    MockGameUnit a, b;
    mgr.SetImmunity(&a, &b, 0);
    EXPECT_TRUE(mgr.ShouldGenerateEvent(&a, &b));
}

// ============================================================
// Update — frame expiry
// ============================================================

TEST(ImmunityManagerTest, ExpiresAfterFrameCount) {
    ImmunityManager mgr;
    MockGameUnit a, b;
    mgr.SetImmunity(&a, &b, 3);
    mgr.Update(); EXPECT_FALSE(mgr.ShouldGenerateEvent(&a, &b));
    mgr.Update(); EXPECT_FALSE(mgr.ShouldGenerateEvent(&a, &b));
    mgr.Update(); EXPECT_TRUE(mgr.ShouldGenerateEvent(&a, &b));
}

TEST(ImmunityManagerTest, UpdateOnEmptyManagerDoesNotCrash) {
    ImmunityManager mgr;
    EXPECT_NO_THROW(mgr.Update());
}

// ============================================================
// Clear / ClearImmunity
// ============================================================

TEST(ImmunityManagerTest, ClearRemovesAllImmunity) {
    ImmunityManager mgr;
    MockGameUnit a, b;
    mgr.SetImmunity(&a, &b, 10);
    mgr.Clear();
    EXPECT_TRUE(mgr.ShouldGenerateEvent(&a, &b));
}

TEST(ImmunityManagerTest, ClearImmunityForOneUnitAllowsEvents) {
    ImmunityManager mgr;
    MockGameUnit a, b;
    mgr.SetImmunity(&a, &b, 10);
    mgr.ClearImmunity(&a);
    EXPECT_TRUE(mgr.ShouldGenerateEvent(&a, &b));
}

TEST(ImmunityManagerTest, ClearImmunityOnUnknownUnitIsNoOp) {
    ImmunityManager mgr;
    MockGameUnit u;
    EXPECT_NO_THROW(mgr.ClearImmunity(&u));
}

TEST(ImmunityManagerTest, ClearImmunityOnNullIsNoOp) {
    ImmunityManager mgr;
    EXPECT_NO_THROW(mgr.ClearImmunity(nullptr));
}

// ============================================================
// Pairwise isolation
// ============================================================

TEST(ImmunityManagerTest, ImmunityIsPairwiseNotGlobal) {
    ImmunityManager mgr;
    MockGameUnit a, b, c;
    mgr.SetImmunity(&a, &b, 5);
    EXPECT_FALSE(mgr.ShouldGenerateEvent(&a, &b));
    EXPECT_TRUE(mgr.ShouldGenerateEvent(&a, &c));
    EXPECT_TRUE(mgr.ShouldGenerateEvent(&b, &c));
}
