#include "PlacementValidator.h"

#include "core/interfaces/IInfluenceEmitter.h"
#include "core/interfaces/IUnitCombat.h"
#include "core/math/ArenaGeometry.h"

#include <algorithm>
#include <cmath>

namespace {

bool IsAlive(const IGameUnit* unit) {
    if (!unit) {
        return false;
    }
    if (const auto* combat = dynamic_cast<const IUnitCombat*>(unit)) {
        return !combat->IsDead();
    }
    return unit->GetCurrentHP() > 0.0f;
}

bool DiscTouches(float x, float y, float ox, float oy, float radius,
                 bool wrap, float arenaW, float arenaH) {
    const float distSq = ArenaGeometry::DistanceSq(x, y, ox, oy, arenaW, arenaH, wrap);
    return distSq <= radius * radius;
}

}  // namespace

bool PlacementValidator::IsValidPlacement(
    float x,
    float y,
    int placingTeamId,
    const std::vector<const IGameUnit*>& units,
    const std::vector<Obstacle>& obstacles,
    float arenaW,
    float arenaH,
    const Rules& rules) const
{
    return IsValidPlacement(x, y, placingTeamId, units, obstacles, arenaW, arenaH, rules, 0.0f);
}

bool PlacementValidator::IsValidPlacement(
    float x,
    float y,
    int placingTeamId,
    const std::vector<const IGameUnit*>& units,
    const std::vector<Obstacle>& obstacles,
    float arenaW,
    float arenaH,
    const Rules& rules,
    float candidateRadius) const
{
    if (!(arenaW > 0.0f) || !(arenaH > 0.0f) || !(candidateRadius >= 0.0f)) {
        return false;
    }
    if (rules.baseSeparationRadius < 0.0f || rules.minPlacementDistance < 0.0f) {
        return false;
    }

    if (rules.wrapBorders) {
        const Vector2 wrapped = ArenaGeometry::WrapPosition(x, y, arenaW, arenaH);
        x = wrapped.x;
        y = wrapped.y;
    } else if (x < candidateRadius || y < candidateRadius
               || x > arenaW - candidateRadius || y > arenaH - candidateRadius) {
        return false;
    }

    if (rules.territoryFilter && !rules.territoryFilter(x, y, placingTeamId)) {
        return false;
    }

    for (const Obstacle& obstacle : obstacles) {
        if (!obstacle.blocksPlacement) {
            continue;
        }
        if (DiscTouches(x, y, obstacle.x, obstacle.y, obstacle.radius + candidateRadius,
                        rules.wrapBorders, arenaW, arenaH)) {
            return false;
        }
    }

    const float floorDistance = std::max(0.0f, rules.minPlacementDistance);
    for (const IGameUnit* unit : units) {
        if (!IsAlive(unit)) {
            continue;
        }
        const bool enemy = unit->GetTeam() != placingTeamId;
        float clearance = rules.baseSeparationRadius;
        if (enemy && rules.enemyClearanceUsesInfluence) {
            if (const auto* emitter = dynamic_cast<const IInfluenceEmitter*>(unit)) {
                clearance = emitter->GetInfluenceRadius();
            }
        }
        clearance = std::max(clearance, floorDistance);
        if (candidateRadius > 0.0f) {
            clearance += candidateRadius + unit->GetRadius();
        }
        if (clearance <= 0.0f) {
            continue;
        }
        const Vector2 pos = unit->GetPosition();
        const float distSq = ArenaGeometry::DistanceSq(
            x, y, pos.x, pos.y, arenaW, arenaH, rules.wrapBorders);
        if (distSq < clearance * clearance) {
            return false;
        }
    }

    return true;
}
