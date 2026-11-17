#include <gtest/gtest.h>

#include "core/gameloop/DeploymentTimer.h"
#include "core/gameloop/MatchState.h"
#include "core/gameloop/ThinkTimer.h"

TEST(DeploymentTimerChargeTest, InitialFractionPreFillsWithoutACharge) {
    DeploymentTimer timer(4.0f, 0.25f);
    EXPECT_FALSE(timer.HasCharge());
    EXPECT_FLOAT_EQ(timer.GetChargeProgress(), 0.25f);
}

TEST(DeploymentTimerChargeTest, FullInitialFractionStartsReady) {
    DeploymentTimer timer(2.0f, 1.0f);
    EXPECT_TRUE(timer.HasCharge());
    EXPECT_EQ(timer.GetCharges(), 1);
    EXPECT_FLOAT_EQ(timer.GetChargeProgress(), 1.0f);
}

TEST(DeploymentTimerChargeTest, GrantChargeAddsOneImmediately) {
    DeploymentTimer timer(5.0f);
    timer.Update(1.0f);
    timer.GrantCharge();
    EXPECT_EQ(timer.GetCharges(), 1);
    EXPECT_FLOAT_EQ(timer.GetChargeProgress(), 1.0f);
}

TEST(ThinkTimerParityTest, ActivateIsIgnoredOnceExhausted) {
    ThinkTimer timer(1.0f);
    timer.Activate();
    timer.Update(1.0f);
    EXPECT_TRUE(timer.IsExhausted());
    EXPECT_FALSE(timer.IsActive());
    timer.Activate();
    EXPECT_FALSE(timer.IsActive());
    EXPECT_FLOAT_EQ(timer.GetBankRemaining(), 0.0f);
}

TEST(MultiTeamMatchStateTest, OneTeamHoldWins) {
    MultiTeamMatchState match;
    match.teamTerritoryPct = {0.0f, 0.8f, 0.1f, 0.1f};
    match.Advance(10.0f);
    EXPECT_EQ(match.winnerTeamId, 1);
    EXPECT_TRUE(match.IsOver());
}

TEST(MultiTeamMatchStateTest, SimultaneousHoldsDraw) {
    MultiTeamMatchState match;
    match.teamTerritoryPct = {0.0f, 0.8f, 0.9f};
    match.Advance(10.0f);
    EXPECT_EQ(match.winnerTeamId, -1);
}

TEST(MultiTeamMatchStateTest, TimeLimitPicksHighestTerritory) {
    MultiTeamMatchState match;
    match.matchTimeLimit = 5.0f;
    match.teamTerritoryPct = {0.0f, 0.2f, 0.4f, 0.4f};
    match.Advance(5.0f);
    EXPECT_EQ(match.winnerTeamId, -1);

    MultiTeamMatchState lead;
    lead.matchTimeLimit = 5.0f;
    lead.teamTerritoryPct = {0.0f, 0.2f, 0.5f, 0.1f};
    lead.Advance(5.0f);
    EXPECT_EQ(lead.winnerTeamId, 2);
}
