#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include "ffi/c_api.h"

namespace {

constexpr float kStep = 1.0f / 30.0f;
constexpr float kMapW = 1600.0f;
constexpr float kMapH = 900.0f;

uint32_t FloatBits(float value)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void RunUpdates(CE_Engine e, int count)
{
    for (int i = 0; i < count; ++i) {
        ASSERT_EQ(ce_engine_update(e, kStep), 1) << "at step " << i;
    }
}

class CApiSnapshotTest : public ::testing::Test {
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

}  // namespace

TEST_F(CApiSnapshotTest, FreshSnapshotAndTerritoryFraction) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);

    CE_GameSnapshot snap{};
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.matchOver, 0);
    EXPECT_EQ(snap.winner, 0);
    EXPECT_EQ(snap.unitCount, 0);
    EXPECT_EQ(FloatBits(snap.mapWidth), FloatBits(kMapW));
    EXPECT_EQ(FloatBits(snap.mapHeight), FloatBits(kMapH));
    EXPECT_EQ(FloatBits(snap.matchTimeElapsed), FloatBits(0.0f));
    EXPECT_EQ(ce_engine_update(e, kStep), 1);
    EXPECT_EQ(FloatBits(ce_territory_get_pct(e, 1)), FloatBits(0.0f));
    EXPECT_EQ(ce_last_result(e), CE_OK);
}

TEST_F(CApiSnapshotTest, TimeoutDrawAtThreeHundredSeconds) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);

    for (int i = 0; i < 8999; ++i) {
        ASSERT_EQ(ce_engine_update(e, kStep), 1) << i;
    }

    CE_GameSnapshot snap{};
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.matchOver, 0);
    EXPECT_EQ(snap.winner, 0);
    EXPECT_EQ(FloatBits(snap.matchTimeElapsed), FloatBits(0x1.2bfb4ep+8f));

    ASSERT_EQ(ce_engine_update(e, kStep), 0);
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.matchOver, 1);
    EXPECT_EQ(snap.winner, 3);
    EXPECT_EQ(FloatBits(snap.matchTimeElapsed), FloatBits(0x1.2c03d6p+8f));

    const float frozenTime = snap.matchTimeElapsed;
    EXPECT_EQ(ce_engine_update(e, kStep), 0);
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(FloatBits(snap.matchTimeElapsed), FloatBits(frozenTime));
}

TEST_F(CApiSnapshotTest, DominanceWinOnHoldBoundary) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);

    RunUpdates(e, 300);
    ASSERT_EQ(ce_unit_spawn(e, 1, 800.0f, 450.0f), 1);
    EXPECT_EQ(FloatBits(ce_territory_get_pct(e, 1)), FloatBits(1.0f));
    EXPECT_EQ(FloatBits(ce_territory_get_pct(e, 2)), FloatBits(0.0f));

    CE_GameSnapshot snap{};
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.winner, 0);

    for (int i = 0; i < 299; ++i) {
        ASSERT_EQ(ce_engine_update(e, kStep), 1);
    }
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.matchOver, 0);
    EXPECT_EQ(snap.winner, 0);

    ASSERT_EQ(ce_engine_update(e, kStep), 0);
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.winner, 1);
    EXPECT_EQ(snap.matchOver, 1);
}

TEST_F(CApiSnapshotTest, SplitTerritoryStaysRunningUntilTimeLimit) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);

    RunUpdates(e, 600);
    ASSERT_EQ(ce_unit_spawn(e, 1, 400.0f, 450.0f), 1);
    ASSERT_EQ(ce_unit_spawn(e, 2, 1200.0f, 450.0f), 1);
    EXPECT_EQ(FloatBits(ce_territory_get_pct(e, 1)), FloatBits(0.5f));
    EXPECT_EQ(FloatBits(ce_territory_get_pct(e, 2)), FloatBits(0.5f));

    RunUpdates(e, 300);

    CE_GameSnapshot snap{};
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.winner, 0);
    EXPECT_EQ(snap.matchOver, 0);
}

TEST_F(CApiSnapshotTest, ResetClearsMatchOutcome) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);

    for (int i = 0; i < 9000; ++i) {
        ce_engine_update(e, kStep);
    }

    CE_GameSnapshot snap{};
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.matchOver, 1);
    EXPECT_EQ(snap.winner, 3);

    ce_engine_reset(e);
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(snap.winner, 0);
    EXPECT_EQ(snap.matchOver, 0);
    EXPECT_EQ(FloatBits(snap.matchTimeElapsed), FloatBits(0.0f));
    EXPECT_EQ(ce_engine_update(e, kStep), 1);
}

TEST_F(CApiSnapshotTest, NullSnapshotGetUnchanged) {
    CE_GameSnapshot snap{};
    EXPECT_EQ(ce_snapshot_get(nullptr, &snap), 0);
    EXPECT_EQ(ce_snapshot_get(nullptr, nullptr), 0);
}
