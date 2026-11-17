#pragma once

#include <vector>

/// Match result enum — shared by DynamicGO win conditions, AscendantExpanse territory,
/// and any future game using the CoreEngine match loop.
enum class MatchResult { None, Team1Wins, Team2Wins, Draw };

/// Canonical match state for any 2-team game using the CoreEngine.
///
/// `team1TerritoryPct` and `team2TerritoryPct` are fractions in [0, 1] (not the
/// TerritoryCalculator 0–100 scale). Callers must assign them before each `Advance`.
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

    void Advance(float dt)
    {
        if (result != MatchResult::None) {
            return;
        }
        if (dt < 0.0f) {
            return;
        }

        matchTimeElapsed += dt;

        if (team1TerritoryPct >= territoryWinThreshold) {
            team1TerritoryHoldTime += dt;
        } else {
            team1TerritoryHoldTime = 0.0f;
        }

        if (team2TerritoryPct >= territoryWinThreshold) {
            team2TerritoryHoldTime += dt;
        } else {
            team2TerritoryHoldTime = 0.0f;
        }

        const bool team1HoldMet = team1TerritoryHoldTime >= territoryHoldDuration;
        const bool team2HoldMet = team2TerritoryHoldTime >= territoryHoldDuration;

        if (team1HoldMet && team2HoldMet) {
            result = MatchResult::Draw;
            return;
        }
        if (team1HoldMet) {
            result = MatchResult::Team1Wins;
            return;
        }
        if (team2HoldMet) {
            result = MatchResult::Team2Wins;
            return;
        }

        if (matchTimeElapsed >= matchTimeLimit) {
            if (team1TerritoryPct > team2TerritoryPct) {
                result = MatchResult::Team1Wins;
            } else if (team2TerritoryPct > team1TerritoryPct) {
                result = MatchResult::Team2Wins;
            } else {
                result = MatchResult::Draw;
            }
        }
    }
};

/// N-team / FFA match state. `teamTerritoryPct[teamId]` is a fraction in [0, 1].
/// Index 0 is unused so team ids match `IGameUnit::GetTeam()`.
/// `winnerTeamId` is 0 while the match is running, -1 on a draw, or the winning id.
struct MultiTeamMatchState {
    float matchTimeElapsed = 0.0f;
    float matchTimeLimit = 300.0f;

    std::vector<float> teamTerritoryPct;

    float territoryWinThreshold = 0.75f;
    float territoryHoldDuration = 10.0f;
    std::vector<float> teamHoldTime;

    int winnerTeamId = 0;

    [[nodiscard]] bool IsOver() const { return winnerTeamId != 0; }

    void Advance(float dt)
    {
        if (IsOver() || dt < 0.0f) {
            return;
        }
        if (teamHoldTime.size() < teamTerritoryPct.size()) {
            teamHoldTime.resize(teamTerritoryPct.size(), 0.0f);
        }

        matchTimeElapsed += dt;

        int holders = 0;
        int lastHolder = 0;
        for (std::size_t i = 1; i < teamTerritoryPct.size(); ++i) {
            if (teamTerritoryPct[i] >= territoryWinThreshold) {
                teamHoldTime[i] += dt;
            } else {
                teamHoldTime[i] = 0.0f;
            }
            if (teamHoldTime[i] >= territoryHoldDuration) {
                ++holders;
                lastHolder = static_cast<int>(i);
            }
        }

        if (holders > 1) {
            winnerTeamId = -1;
            return;
        }
        if (holders == 1) {
            winnerTeamId = lastHolder;
            return;
        }

        if (matchTimeElapsed < matchTimeLimit) {
            return;
        }

        int bestTeam = 0;
        float bestPct = -1.0f;
        int tied = 0;
        for (std::size_t i = 1; i < teamTerritoryPct.size(); ++i) {
            if (teamTerritoryPct[i] > bestPct) {
                bestPct = teamTerritoryPct[i];
                bestTeam = static_cast<int>(i);
                tied = 1;
            } else if (teamTerritoryPct[i] == bestPct) {
                ++tied;
            }
        }
        winnerTeamId = (tied == 1) ? bestTeam : -1;
    }
};
