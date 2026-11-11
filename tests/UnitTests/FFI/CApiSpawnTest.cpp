#include <gtest/gtest.h>
#include <cstring>
#include "ffi/c_api.h"

namespace {

constexpr float kStep = 1.0f / 30.0f;

class CApiSpawnTest : public ::testing::Test {
protected:
    CE_Engine e = nullptr;

    void SetUp() override { e = ce_engine_create(1600.0f, 900.0f, 1); }
    void TearDown() override
    {
        if (e) {
            ce_engine_destroy(e);
            e = nullptr;
        }
    }
};

} // namespace

TEST_F(CApiSpawnTest, FreshEngineHasNoChargeUntilGrantOr300Steps) {
    EXPECT_EQ(ce_timer_has_charge(e, 1), 0);
    EXPECT_EQ(ce_timer_has_charge(e, 2), 0);
    EXPECT_FLOAT_EQ(ce_timer_get_progress(e, 1), 0.0f);
    EXPECT_EQ(ce_unit_spawn(e, 1, 800.0f, 450.0f), 0);
    EXPECT_EQ(ce_last_result(e), CE_ERR_NO_CHARGE);
    EXPECT_EQ(ce_unit_count(e), 0);
}

TEST_F(CApiSpawnTest, NullHandleSpawnAndLastResult) {
    EXPECT_EQ(ce_unit_spawn(nullptr, 1, 0.0f, 0.0f), 0);
    EXPECT_EQ(ce_last_result(nullptr), CE_ERR_NULL);
}

TEST_F(CApiSpawnTest, InvalidTeamDoesNotSpendCharge) {
    ASSERT_EQ(ce_timer_grant_charge(e, 2), 1);
    EXPECT_EQ(ce_unit_spawn(e, 99, 1.0f, 1.0f), 0);
    EXPECT_EQ(ce_last_result(e), CE_ERR_INVALID_TEAM);
    EXPECT_EQ(ce_unit_count(e), 0);
    EXPECT_EQ(ce_timer_has_charge(e, 2), 1);

    EXPECT_EQ(ce_unit_spawn(e, 2, 400.0f, 450.0f), 1);
    EXPECT_EQ(ce_last_result(e), CE_OK);

    CE_UnitState st{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &st), 1);
    EXPECT_EQ(st.team, 2);
    EXPECT_FLOAT_EQ(st.x, 400.0f);
    EXPECT_FLOAT_EQ(st.y, 450.0f);
    EXPECT_FLOAT_EQ(st.hp, 100.0f);
    EXPECT_FLOAT_EQ(st.maxHp, 100.0f);
    EXPECT_FLOAT_EQ(st.radius, 10.0f);

    EXPECT_EQ(ce_timer_has_charge(e, 2), 0);
    EXPECT_EQ(ce_timer_has_charge(e, 1), 0);
}

TEST_F(CApiSpawnTest, GrantTeam1DoesNotChargeTeam2) {
    CE_Engine eng = ce_engine_create(1600.0f, 900.0f, 2);
    ASSERT_EQ(ce_timer_grant_charge(eng, 1), 1);
    EXPECT_EQ(ce_unit_spawn(eng, 2, 100.0f, 100.0f), 0);
    EXPECT_EQ(ce_last_result(eng), CE_ERR_NO_CHARGE);
    EXPECT_EQ(ce_unit_spawn(eng, 1, 100.0f, 100.0f), 1);
    ce_engine_destroy(eng);
}

TEST_F(CApiSpawnTest, ChargeAfter300LogicSteps) {
    for (int i = 0; i < 299; ++i) {
        ASSERT_EQ(ce_engine_update(e, kStep), 1);
    }
    EXPECT_EQ(ce_timer_has_charge(e, 1), 0);
    EXPECT_FLOAT_EQ(ce_timer_get_progress(e, 1), 0x1.fe4b5ap-1f);

    ASSERT_EQ(ce_engine_update(e, kStep), 1);
    EXPECT_EQ(ce_timer_has_charge(e, 1), 1);
    EXPECT_FLOAT_EQ(ce_timer_get_progress(e, 1), 1.0f);
    EXPECT_EQ(ce_unit_spawn(e, 1, 50.0f, 50.0f), 1);
}

TEST_F(CApiSpawnTest, SingleTenSecondUpdateDoesNotGrantCharge) {
    ASSERT_EQ(ce_engine_update(e, 10.0f), 1);
    EXPECT_EQ(ce_timer_has_charge(e, 1), 0);

    CE_GameSnapshot snap{};
    ASSERT_EQ(ce_snapshot_get(e, &snap), 1);
    EXPECT_FLOAT_EQ(snap.matchTimeElapsed, 0x1.3eef18p+3f);
}

TEST_F(CApiSpawnTest, FfiVersionString) {
    EXPECT_STREQ(ce_ffi_version(), "1.1.0");
}
