#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
#include "ffi/c_api.h"

namespace {

constexpr float kStep = 1.0f / 30.0f;
constexpr uint64_t kSeed12345 = 12345u;
constexpr uint64_t kSeed42 = 42u;

uint32_t FloatBits(float value)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void RunWanderSequence(CE_Engine e)
{
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 800.0f, 450.0f), 1);
    ASSERT_EQ(ce_unit_set_behavior(e, 0, "Wander"), 1);
    for (int i = 0; i < 30; ++i) {
        ASSERT_EQ(ce_engine_update(e, kStep), 1);
    }
}

class CApiResetTest : public ::testing::Test {
protected:
    CE_Engine e = nullptr;

    void TearDown() override
    {
        if (e) {
            ce_engine_destroy(e);
            e = nullptr;
        }
    }
};

} // namespace

TEST_F(CApiResetTest, CreateReportsSeedAndResetPreservesRngSeed42) {
    e = ce_engine_create(1600.0f, 900.0f, kSeed42);
    EXPECT_EQ(ce_engine_get_seed(e), kSeed42);

    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 800.0f, 450.0f), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);
    EXPECT_EQ(ce_unit_count(e), 1);

    CE_GameSnapshot snap{};
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(FloatBits(snap.matchTimeElapsed), 0x3d088889u);

    ce_engine_reset(e);

    EXPECT_EQ(ce_engine_get_seed(e), kSeed42);
    EXPECT_EQ(ce_unit_count(e), 0);
    EXPECT_EQ(ce_timer_has_charge(e, 1), 0);
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(FloatBits(snap.matchTimeElapsed), FloatBits(0.0f));
    EXPECT_EQ(FloatBits(snap.mapWidth), FloatBits(1600.0f));
    EXPECT_EQ(FloatBits(snap.mapHeight), FloatBits(900.0f));
    EXPECT_EQ(ce_engine_update(e, kStep), 1);
}

TEST_F(CApiResetTest, SeedZeroRoundTripsThroughReset) {
    e = ce_engine_create(1600.0f, 900.0f, 0);
    EXPECT_EQ(ce_engine_get_seed(e), 0u);
    ce_engine_reset(e);
    EXPECT_EQ(ce_engine_get_seed(e), 0u);
}

TEST_F(CApiResetTest, GetSeedNullEngineReturnsZero) {
    EXPECT_EQ(ce_engine_get_seed(nullptr), 0u);
}

TEST_F(CApiResetTest, GetSeedUsesLiveRngNotStaleContext) {
    e = ce_engine_create(1600.0f, 900.0f, kSeed42);
    EXPECT_EQ(ce_engine_get_seed(e), kSeed42);
    ce_engine_reset(e);
    EXPECT_EQ(ce_engine_get_seed(e), kSeed42);
}

TEST_F(CApiResetTest, NoCrossTalkBetweenEngines) {
    e = ce_engine_create(1600.0f, 900.0f, kSeed42);
    CE_Engine other = ce_engine_create(1600.0f, 900.0f, 7u);
    ASSERT_NE(other, nullptr);

    ce_engine_reset(e);
    EXPECT_EQ(ce_engine_get_seed(other), 7u);

    ce_engine_destroy(other);
}

TEST_F(CApiResetTest, FfiVersionIs120) {
    EXPECT_STREQ(ce_ffi_version(), "1.2.0");
}

TEST_F(CApiResetTest, ReplayMatchesFreshAfterResetWithSeed12345) {
    CE_Engine fresh = ce_engine_create(1600.0f, 900.0f, kSeed12345);
    ASSERT_NE(fresh, nullptr);
    RunWanderSequence(fresh);

    CE_UnitState freshUnit{};
    ASSERT_EQ(ce_unit_get_state(fresh, 0, &freshUnit), 1);
    CE_GameSnapshot freshSnap{};
    ASSERT_EQ(ce_snapshot_get(fresh, &freshSnap), 1);

    e = ce_engine_create(1600.0f, 900.0f, kSeed12345);
    ASSERT_NE(e, nullptr);
    RunWanderSequence(e);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);
    ce_engine_reset(e);
    RunWanderSequence(e);

    CE_UnitState resetUnit{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &resetUnit), 1);
    CE_GameSnapshot resetSnap{};
    ASSERT_EQ(ce_snapshot_get(e, &resetSnap), 1);

    EXPECT_EQ(resetUnit.team, freshUnit.team);
    EXPECT_EQ(FloatBits(resetUnit.x), FloatBits(freshUnit.x));
    EXPECT_EQ(FloatBits(resetUnit.y), FloatBits(freshUnit.y));
    EXPECT_EQ(FloatBits(resetUnit.vx), FloatBits(freshUnit.vx));
    EXPECT_EQ(FloatBits(resetUnit.vy), FloatBits(freshUnit.vy));
    EXPECT_EQ(FloatBits(resetUnit.hp), FloatBits(freshUnit.hp));
    EXPECT_EQ(FloatBits(resetUnit.maxHp), FloatBits(freshUnit.maxHp));
    EXPECT_EQ(FloatBits(resetUnit.radius), FloatBits(freshUnit.radius));
    EXPECT_EQ(resetSnap.unitCount, freshSnap.unitCount);
    EXPECT_EQ(FloatBits(resetSnap.matchTimeElapsed), FloatBits(freshSnap.matchTimeElapsed));
    EXPECT_EQ(FloatBits(resetSnap.mapWidth), FloatBits(freshSnap.mapWidth));
    EXPECT_EQ(FloatBits(resetSnap.mapHeight), FloatBits(freshSnap.mapHeight));

    ce_engine_destroy(fresh);
}
