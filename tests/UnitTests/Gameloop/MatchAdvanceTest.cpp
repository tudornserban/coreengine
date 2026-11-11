#include <gtest/gtest.h>
#include "core/gameloop/MatchState.h"
#include "core/gameloop/GameLogicEngine.h"

TEST(MatchAdvanceTest, Team1DominanceWinAtTenSeconds) {
    MatchState m;
    m.team1TerritoryPct = 0.75f;
    m.Advance(10.0f);
    EXPECT_EQ(m.result, MatchResult::Team1Wins);
}

TEST(MatchAdvanceTest, DominanceHoldOneStepBeforeTenSeconds) {
    constexpr float kStep = 1.0f / 30.0f;
    MatchState m;
    m.team1TerritoryPct = 0.75f;
    for (int i = 0; i < 299; ++i) {
        m.Advance(kStep);
    }
    EXPECT_EQ(m.result, MatchResult::None);
    EXPECT_LT(m.team1TerritoryHoldTime, m.territoryHoldDuration);
    m.Advance(kStep);
    EXPECT_EQ(m.result, MatchResult::Team1Wins);
}

TEST(MatchAdvanceTest, EqualTerritoryBelowThresholdDoesNotWinOnHold) {
    MatchState m;
    m.team1TerritoryPct = 0.5f;
    m.team2TerritoryPct = 0.5f;
    m.Advance(10.0f);
    EXPECT_EQ(m.result, MatchResult::None);
}

TEST(MatchAdvanceTest, TimeLimitExactTieIsDraw) {
    MatchState m;
    m.Advance(299.0f);
    EXPECT_EQ(m.result, MatchResult::None);
    m.Advance(1.0f);
    EXPECT_EQ(m.result, MatchResult::Draw);
}

TEST(MatchAdvanceTest, TimeLimitHigherShareWins) {
    MatchState m;
    m.team2TerritoryPct = 0.6f;
    m.team1TerritoryPct = 0.4f;
    m.Advance(300.0f);
    EXPECT_EQ(m.result, MatchResult::Team2Wins);
    m.Advance(10.0f);
    EXPECT_EQ(m.result, MatchResult::Team2Wins);
}

TEST(MatchAdvanceTest, TimeLimitTeam1HigherShareWins) {
    MatchState m;
    m.team1TerritoryPct = 0.55f;
    m.team2TerritoryPct = 0.45f;
    m.Advance(300.0f);
    EXPECT_EQ(m.result, MatchResult::Team1Wins);
}

TEST(MatchAdvanceTest, LossOfDominanceResetsHoldTimer) {
    MatchState m;
    m.team1TerritoryPct = 0.75f;
    m.Advance(5.0f);
    EXPECT_GT(m.team1TerritoryHoldTime, 0.0f);
    m.team1TerritoryPct = 0.5f;
    m.Advance(5.0f);
    EXPECT_EQ(m.team1TerritoryHoldTime, 0.0f);
    EXPECT_EQ(m.result, MatchResult::None);
    m.team1TerritoryPct = 0.75f;
    m.Advance(10.0f);
    EXPECT_EQ(m.result, MatchResult::Team1Wins);
}

TEST(MatchAdvanceTest, SimultaneousDominanceHoldIsDraw) {
    MatchState m;
    m.team1TerritoryPct = 0.8f;
    m.team2TerritoryPct = 0.8f;
    m.Advance(10.0f);
    EXPECT_EQ(m.result, MatchResult::Draw);
}

TEST(GameLogicEngineTest, ShouldEndCallbackStopsNormalLoop) {
    GameLogicEngine engine(1.0f / 30.0f);
    int counter = 0;
    engine.SetUpdateCallback([&](float /*dt*/) { ++counter; });
    engine.SetShouldEndCallback([&]() { return counter >= 1; });
    engine.Update(1.0f);
    EXPECT_EQ(counter, 1);
    EXPECT_EQ(engine.GetGameTime(), 1.0f / 30.0f);
}
