#include "TerritoryCalculator.h"
#include <limits>

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
