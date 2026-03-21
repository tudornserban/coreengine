#include <gtest/gtest.h>
#include "core/ai/BehaviorFactory.h"
#include "core/ai/Behaviors/SeekBehavior.h"
#include "core/ai/Behaviors/FleeBehavior.h"
#include "tests/TestHelpers/MockBehavior.h"

class BehaviorFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register builtins fresh for each test
        BehaviorFactory::RegisterBuiltins();
    }

    void TearDown() override {
        // Clean up any test-specific keys
        BehaviorFactory::Unregister("TestBehavior");
        BehaviorFactory::Unregister("CustomKey");
    }
};

TEST_F(BehaviorFactoryTest, BuiltinSeekIsRegistered) {
    EXPECT_TRUE(BehaviorFactory::IsRegistered("Seek"));
}

TEST_F(BehaviorFactoryTest, BuiltinFleeIsRegistered) {
    EXPECT_TRUE(BehaviorFactory::IsRegistered("Flee"));
}

TEST_F(BehaviorFactoryTest, BuiltinWanderIsRegistered) {
    EXPECT_TRUE(BehaviorFactory::IsRegistered("Wander"));
}

TEST_F(BehaviorFactoryTest, CreateBuiltinReturnsNonNull) {
    auto b = BehaviorFactory::Create("Seek");
    ASSERT_NE(b, nullptr);
    EXPECT_STREQ(b->GetName(), "Seek");
}

TEST_F(BehaviorFactoryTest, CreateUnknownKeyReturnsNullptr) {
    EXPECT_EQ(BehaviorFactory::Create("__nonexistent__"), nullptr);
}

TEST_F(BehaviorFactoryTest, RegisterAndCreate) {
    BehaviorFactory::Register("TestBehavior",
        [] { return std::make_unique<MockBehavior>("TestBehavior"); });
    EXPECT_TRUE(BehaviorFactory::IsRegistered("TestBehavior"));
    auto b = BehaviorFactory::Create("TestBehavior");
    ASSERT_NE(b, nullptr);
    EXPECT_STREQ(b->GetName(), "TestBehavior");
}

TEST_F(BehaviorFactoryTest, CreateReturnsNewInstanceEachTime) {
    auto b1 = BehaviorFactory::Create("Seek");
    auto b2 = BehaviorFactory::Create("Seek");
    ASSERT_NE(b1, nullptr);
    ASSERT_NE(b2, nullptr);
    EXPECT_NE(b1.get(), b2.get());
}

TEST_F(BehaviorFactoryTest, ReRegisterOverwritesPreviousCreator) {
    BehaviorFactory::Register("CustomKey",
        [] { return std::make_unique<MockBehavior>("First"); });
    BehaviorFactory::Register("CustomKey",
        [] { return std::make_unique<MockBehavior>("Second"); });
    auto b = BehaviorFactory::Create("CustomKey");
    ASSERT_NE(b, nullptr);
    EXPECT_STREQ(b->GetName(), "Second");
}

TEST_F(BehaviorFactoryTest, UnregisterRemovesKey) {
    BehaviorFactory::Register("CustomKey",
        [] { return std::make_unique<MockBehavior>("tmp"); });
    EXPECT_TRUE(BehaviorFactory::IsRegistered("CustomKey"));
    BehaviorFactory::Unregister("CustomKey");
    EXPECT_FALSE(BehaviorFactory::IsRegistered("CustomKey"));
}

TEST_F(BehaviorFactoryTest, GetRegisteredKeysContainsBuiltins) {
    auto keys = BehaviorFactory::GetRegisteredKeys();
    auto it = std::find(keys.begin(), keys.end(), "Seek");
    EXPECT_NE(it, keys.end());
}
