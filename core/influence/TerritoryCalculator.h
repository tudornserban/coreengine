#pragma once

#include <vector>

class IGameUnit;

/// Voronoi-based territory ownership calculator.
///
/// Samples a grid of the arena and assigns each cell to the nearest unit.
/// Returns the fraction of cells owned by each team.
///
/// Used by DynamicGO win conditions and AscendantExpanse map control.
class TerritoryCalculator {
public:
    struct Arena {
        float width;
        float height;
    };

    struct Result {
        float team1Pct;
        float team2Pct;
    };

    /// Grid resolution — increase for more accurate but slower calculation.
    static constexpr int GRID_COLS = 32;
    static constexpr int GRID_ROWS = 32;

    /// Calculate territory ownership from all units (organized by team ID 1 and 2).
    /// Units with other team IDs are ignored.
    [[nodiscard]] Result Calculate(
        const std::vector<IGameUnit*>& units,
        const Arena& arena) const;

    /// Convenience overload for unit lists already split by team.
    template<typename UnitT>
    [[nodiscard]] Result CalculateFromTeams(
        const std::vector<UnitT>& team1Units,
        const std::vector<UnitT>& team2Units,
        const Arena& arena) const;
};
