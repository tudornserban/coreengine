#pragma once

#include "core/interfaces/IGameUnit.h"
#include "core/map/Obstacle.h"

#include <cstdint>
#include <vector>

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

    enum class TerritoryCellState : uint8_t {
        Neutral,
        Passive,
        Active,
        Contested,
        Obstacle
    };

    /// `owner` 0 is neutral. `pendingOwner` is the team accumulating a timed capture.
    struct TerritoryCell {
        int8_t owner = 0;
        TerritoryCellState state = TerritoryCellState::Neutral;
        float captureProgress = 0.0f;
        int8_t pendingOwner = 0;
    };

    struct GridUpdate {
        int cols = GRID_COLS;
        int rows = GRID_ROWS;
        /// 0 captures on the first covering update. Above 0, progress fills at dt/seconds.
        float captureTimeSeconds = 0.0f;
        float dt = 0.0f;
        bool wrapBorders = false;
    };

    /// Calculate territory ownership from all units (organized by team ID 1 and 2).
    /// Units with other team IDs are ignored.
    /// Percentages are on the 0–100 scale. This path does not use influence radius.
    [[nodiscard]] Result Calculate(
        const std::vector<IGameUnit*>& units,
        const Arena& arena) const;

    /// N-team influence grid. Dead units and units without `IInfluenceEmitter` do
    /// not claim cells. `blocksInfluence` obstacles mask cells. Uncovered owned
    /// cells stay `Passive` so a game layer can apply its own decay rules.
    void UpdateGrid(
        const std::vector<const IGameUnit*>& units,
        const std::vector<Obstacle>& obstacles,
        const Arena& arena,
        const GridUpdate& update);

    [[nodiscard]] const std::vector<TerritoryCell>& GetGrid() const { return grid_; }
    [[nodiscard]] int GridCols() const { return gridCols_; }
    [[nodiscard]] int GridRows() const { return gridRows_; }

    /// Convenience overload for unit lists already split by team.
    template<typename UnitT>
    [[nodiscard]] Result CalculateFromTeams(
        const std::vector<UnitT>& team1Units,
        const std::vector<UnitT>& team2Units,
        const Arena& arena) const;

private:
    std::vector<TerritoryCell> grid_;
    int gridCols_ = 0;
    int gridRows_ = 0;
};
