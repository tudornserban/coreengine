#pragma once

/// Match result enum — shared by DynamicGO win conditions, AscendantExpanse territory,
/// and any future game using the CoreEngine match loop.
enum class MatchResult { None, Team1Wins, Team2Wins, Draw };

/// Canonical match state for any 2-team game using the CoreEngine.
struct MatchState {
    float matchTimeElapsed       = 0.0f;
    float matchTimeLimit         = 300.0f;

    float team1TerritoryPct      = 0.0f;
    float team2TerritoryPct      = 0.0f;

    float territoryWinThreshold  = 0.75f;
    float territoryHoldDuration  = 10.0f;
    float team1TerritoryHoldTime = 0.0f;
    float team2TerritoryHoldTime = 0.0f;

    MatchResult result            = MatchResult::None;

    [[nodiscard]] bool IsOver() const { return result != MatchResult::None; }
};
