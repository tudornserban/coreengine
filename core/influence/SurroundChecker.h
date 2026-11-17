#pragma once

#include "core/interfaces/IGameUnit.h"
#include "core/map/Obstacle.h"

#include <vector>

/// Circumference sample check ported from DynamicGO `InfluenceMap::IsSurrounded`.
/// A sample counts as enclosed when it lies outside a non-wrapping arena, inside
/// a `blocksInfluence` obstacle, or inside an enemy influence disc that no other
/// friendly disc also covers. Every sample must be enclosed.
class SurroundChecker {
public:
    static constexpr int kSampleCount = 32;

    [[nodiscard]] static bool IsSurrounded(
        const IGameUnit* target,
        const std::vector<const IGameUnit*>& allUnits,
        const std::vector<Obstacle>& obstacles,
        float arenaW,
        float arenaH,
        bool wrapBorders);
};
