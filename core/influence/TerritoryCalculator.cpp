#include "TerritoryCalculator.h"

#include "core/interfaces/IInfluenceEmitter.h"
#include "core/interfaces/IUnitCombat.h"
#include "core/math/ArenaGeometry.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

TerritoryCalculator::Result TerritoryCalculator::Calculate(
    const std::vector<IGameUnit*>& units,
    const Arena& arena) const
{
    if (units.empty()) return {0.0f, 0.0f};

    const float cellW = arena.width  / static_cast<float>(GRID_COLS);
    const float cellH = arena.height / static_cast<float>(GRID_ROWS);

    int t1Count = 0, t2Count = 0;
    const int total = GRID_COLS * GRID_ROWS;

    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            const float sx = (col + 0.5f) * cellW;
            const float sy = (row + 0.5f) * cellH;

            float bestDist = std::numeric_limits<float>::max();
            int   bestTeam = 0;

            for (const auto* u : units) {
                if (!u) continue;
                float dx = sx - u->GetPosition().x;
                float dy = sy - u->GetPosition().y;
                float d  = dx * dx + dy * dy;
                if (d < bestDist) {
                    bestDist = d;
                    bestTeam = u->GetTeam();
                }
            }

            if      (bestTeam == 1) ++t1Count;
            else if (bestTeam == 2) ++t2Count;
        }
    }

    const float scale = 100.0f / static_cast<float>(total);
    return {
        static_cast<float>(t1Count) * scale,
        static_cast<float>(t2Count) * scale
    };
}

namespace {

bool UnitClaimsTerritory(const IGameUnit* unit) {
    if (!unit) {
        return false;
    }
    if (const auto* combat = dynamic_cast<const IUnitCombat*>(unit)) {
        if (combat->IsDead()) {
            return false;
        }
    }
    const int team = unit->GetTeam();
    if (team <= 0 || team > 127) {
        return false;
    }
    return dynamic_cast<const IInfluenceEmitter*>(unit) != nullptr;
}

}  // namespace

void TerritoryCalculator::UpdateGrid(
    const std::vector<const IGameUnit*>& units,
    const std::vector<Obstacle>& obstacles,
    const Arena& arena,
    const GridUpdate& update)
{
    const int cols = std::max(1, update.cols);
    const int rows = std::max(1, update.rows);
    if (gridCols_ != cols || gridRows_ != rows || static_cast<int>(grid_.size()) != cols * rows) {
        gridCols_ = cols;
        gridRows_ = rows;
        grid_.assign(static_cast<std::size_t>(cols * rows), TerritoryCell{});
    }

    const float cellW = arena.width / static_cast<float>(cols);
    const float cellH = arena.height / static_cast<float>(rows);
    const bool wrap = update.wrapBorders;
    const float dt = update.dt > 0.0f ? update.dt : 0.0f;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const float sx = (static_cast<float>(col) + 0.5f) * cellW;
            const float sy = (static_cast<float>(row) + 0.5f) * cellH;
            TerritoryCell& cell = grid_[static_cast<std::size_t>(row * cols + col)];

            bool masked = false;
            for (const Obstacle& obstacle : obstacles) {
                if (!obstacle.blocksInfluence) {
                    continue;
                }
                const float distSq = ArenaGeometry::DistanceSq(
                    sx, sy, obstacle.x, obstacle.y, arena.width, arena.height, wrap);
                if (distSq <= obstacle.radius * obstacle.radius) {
                    masked = true;
                    break;
                }
            }
            if (masked) {
                cell.owner = 0;
                cell.pendingOwner = 0;
                cell.captureProgress = 0.0f;
                cell.state = TerritoryCellState::Obstacle;
                continue;
            }

            std::vector<int> covering;
            for (const IGameUnit* unit : units) {
                if (!UnitClaimsTerritory(unit)) {
                    continue;
                }
                const auto* emitter = dynamic_cast<const IInfluenceEmitter*>(unit);
                const float radius = emitter->GetInfluenceRadius();
                const Vector2 pos = unit->GetPosition();
                const float distSq = ArenaGeometry::DistanceSq(
                    sx, sy, pos.x, pos.y, arena.width, arena.height, wrap);
                if (distSq > radius * radius) {
                    continue;
                }
                const int team = unit->GetTeam();
                if (std::find(covering.begin(), covering.end(), team) == covering.end()) {
                    covering.push_back(team);
                }
            }

            if (covering.size() >= 2) {
                cell.state = TerritoryCellState::Contested;
                cell.pendingOwner = 0;
                continue;
            }

            if (covering.empty()) {
                cell.pendingOwner = 0;
                if (cell.owner == 0) {
                    cell.state = TerritoryCellState::Neutral;
                    cell.captureProgress = 0.0f;
                } else {
                    cell.state = TerritoryCellState::Passive;
                }
                continue;
            }

            const int team = covering.front();
            const auto teamId = static_cast<int8_t>(team);
            cell.state = TerritoryCellState::Active;
            if (cell.owner == teamId) {
                cell.pendingOwner = teamId;
                cell.captureProgress = 1.0f;
                continue;
            }

            if (cell.pendingOwner != teamId) {
                cell.pendingOwner = teamId;
                cell.captureProgress = 0.0f;
            }
            if (update.captureTimeSeconds <= 0.0f) {
                cell.owner = teamId;
                cell.captureProgress = 1.0f;
                continue;
            }
            cell.captureProgress += dt / update.captureTimeSeconds;
            if (cell.captureProgress >= 1.0f) {
                cell.owner = teamId;
                cell.captureProgress = 1.0f;
                cell.pendingOwner = teamId;
            }
        }
    }
}