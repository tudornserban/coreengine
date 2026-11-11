#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
#include <memory>
#include "ffi/c_api.h"
#include "core/gamestate/RTSGameState.h"

namespace {

constexpr float kStep = 1.0f / 30.0f;
constexpr uint64_t kSeed12345 = 12345u;
constexpr uint64_t kSeed42 = 42u;
constexpr uint64_t kSeedAbove32Bits = 4294967338u; /* 2^32 + 42 */
constexpr uint32_t kFirstDrawSeed42 = 1250496027u;
constexpr uint32_t kFirstDrawSeed0 = 12345u;

// ffi/c_api.cpp EngineContext — `gameState` must remain the first data member.
struct EngineContextLayoutProbe {
    std::unique_ptr<RTSGameState> gameState;
};

static_assert(offsetof(EngineContextLayoutProbe, gameState) == 0,
              "gameState must be first EngineContext member — keep in sync with ffi/c_api.cpp");

uint32_t FloatBits(float value)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

RTSGameState* GameStateFromEngineHandle(CE_Engine engine)
{
    if (!engine) {
        return nullptr;
    }
    auto& gameStateSlot = *reinterpret_cast<std::unique_ptr<RTSGameState>*>(engine);
    return gameStateSlot.get();
}

uint32_t FirstRngDrawFromEngineHandle(CE_Engine engine)
{
    RTSGameState* gameState = GameStateFromEngineHandle(engine);
    if (!gameState) {
        return 0;
    }
    return gameState->GetRNG().NextUInt32();
}

constexpr int kWanderOracleSteps = 90;  // >61 so directionChangeTime (2 s) elapses and RNG draws

void RunWanderOracleSequence(CE_Engine eng)
{
    ASSERT_EQ(ce_timer_grant_charge(eng, 1), 1);
    ASSERT_EQ(ce_unit_spawn(eng, 1, 800.0f, 450.0f), 1);
    ASSERT_EQ(ce_unit_set_behavior(eng, 0, "Wander"), 1);
    for (int i = 0; i < kWanderOracleSteps; ++i) {
        ASSERT_EQ(ce_engine_update(eng, kStep), 1);
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

TEST_F(CApiResetTest, LiveRngFirstDrawAfterResetSeed42) {
    e = ce_engine_create(1600.0f, 900.0f, kSeed42);
    ce_engine_reset(e);
    EXPECT_EQ(FirstRngDrawFromEngineHandle(e), kFirstDrawSeed42);
}

TEST_F(CApiResetTest, LiveRngFirstDrawSeedZeroBeforeAndAfterReset) {
    e = ce_engine_create(1600.0f, 900.0f, 0);
    EXPECT_EQ(FirstRngDrawFromEngineHandle(e), kFirstDrawSeed0);
    ce_engine_reset(e);
    EXPECT_EQ(FirstRngDrawFromEngineHandle(e), kFirstDrawSeed0);
}

TEST_F(CApiResetTest, GetSeedMatchesLiveRngNotContextFieldOnly) {
    e = ce_engine_create(1600.0f, 900.0f, kSeed42);
    ce_engine_reset(e);
    EXPECT_EQ(ce_engine_get_seed(e), kSeed42);
    EXPECT_EQ(FirstRngDrawFromEngineHandle(e), kFirstDrawSeed42);
    EXPECT_EQ(GameStateFromEngineHandle(e)->GetRNG().GetSeed(), kSeed42);
}

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
    EXPECT_EQ(ce_timer_has_charge(e, 2), 0);
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_EQ(FloatBits(snap.matchTimeElapsed), FloatBits(0.0f));
    EXPECT_EQ(FloatBits(snap.mapWidth), FloatBits(1600.0f));
    EXPECT_EQ(FloatBits(snap.mapHeight), FloatBits(900.0f));
    EXPECT_EQ(ce_engine_update(e, kStep), 1);
}

TEST_F(CApiResetTest, ResetClearsTeam2DeploymentTimer) {
    e = ce_engine_create(1600.0f, 900.0f, kSeed42);
    ASSERT_EQ(ce_timer_grant_charge(e, 2), 1);
    EXPECT_EQ(ce_timer_has_charge(e, 2), 1);
    ce_engine_reset(e);
    EXPECT_EQ(ce_timer_has_charge(e, 2), 0);
}

TEST_F(CApiResetTest, SeedAbove32BitsRoundTripsThroughReset) {
    e = ce_engine_create(1600.0f, 900.0f, kSeedAbove32Bits);
    EXPECT_EQ(ce_engine_get_seed(e), kSeedAbove32Bits);
    ce_engine_reset(e);
    EXPECT_EQ(ce_engine_get_seed(e), kSeedAbove32Bits);
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

TEST_F(CApiResetTest, ResetNullEngineIsNoOp) {
    ASSERT_NO_FATAL_FAILURE(ce_engine_reset(nullptr));
}

TEST_F(CApiResetTest, NoCrossTalkBetweenEngines) {
    e = ce_engine_create(1600.0f, 900.0f, kSeed42);
    CE_Engine other = ce_engine_create(1600.0f, 900.0f, 7u);
    ASSERT_NE(other, nullptr);

    ce_engine_reset(e);
    EXPECT_EQ(ce_engine_get_seed(other), 7u);

    ce_engine_destroy(other);
}

TEST_F(CApiResetTest, FfiVersionIs130) {
    EXPECT_STREQ(ce_ffi_version(), "1.3.0");
}

// RNG oracle: Wander draws RNG at first directionChangeTime (2 s ≈ step 61); 90 steps pins post-draw state.
TEST_F(CApiResetTest, WanderReplayAfterResetMatchesFresh_Seed12345_RngOracle) {
    CE_Engine fresh = ce_engine_create(1600.0f, 900.0f, kSeed12345);
    ASSERT_NE(fresh, nullptr);
    RunWanderOracleSequence(fresh);

    CE_UnitState freshUnit{};
    ASSERT_EQ(ce_unit_get_state(fresh, 0, &freshUnit), 1);
    CE_GameSnapshot freshSnap{};
    ASSERT_EQ(ce_snapshot_get(fresh, &freshSnap), 1);

    // Pin exact fresh-run literals (seed 12345, 90 × 1/30 s after Wander set).
    EXPECT_EQ(FloatBits(freshUnit.x), 0x44700547u);
    EXPECT_EQ(FloatBits(freshUnit.y), 0x43cad8dau);
    EXPECT_EQ(FloatBits(freshUnit.vx), 0xc1e4db68u);
    EXPECT_EQ(FloatBits(freshUnit.vy), 0xc287baa2u);

    e = ce_engine_create(1600.0f, 900.0f, kSeed12345);
    ASSERT_NE(e, nullptr);
    RunWanderOracleSequence(e);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);
    ce_engine_reset(e);
    RunWanderOracleSequence(e);

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
