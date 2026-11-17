#include "SurroundChecker.h"

#include "core/interfaces/IInfluenceEmitter.h"
#include "core/math/ArenaGeometry.h"

#include <cmath>

namespace {

constexpr float kTwoPi = 6.28318530717958647692f;

const IInfluenceEmitter* EmitterOf(const IGameUnit* unit) {
    return dynamic_cast<const IInfluenceEmitter*>(unit);
}

bool InsideDisc(float px, float py, const IGameUnit* unit, float arenaW, float arenaH, bool wrap) {
    const IInfluenceEmitter* emitter = EmitterOf(unit);
    if (!emitter) {
        return false;
    }
    const float radius = emitter->GetInfluenceRadius();
    const Vector2 pos = unit->GetPosition();
    const float distSq = ArenaGeometry::DistanceSq(px, py, pos.x, pos.y, arenaW, arenaH, wrap);
    return distSq <= radius * radius;
}

}  // namespace

bool SurroundChecker::IsSurrounded(
    const IGameUnit* target,
    const std::vector<const IGameUnit*>& allUnits,
    const std::vector<Obstacle>& obstacles,
    float arenaW,
    float arenaH,
    bool wrapBorders)
{
    if (!target) {
        return false;
    }
    const IInfluenceEmitter* targetEmitter = EmitterOf(target);
    if (!targetEmitter) {
        return false;
    }

    const Vector2 center = target->GetPosition();
    const float radius = targetEmitter->GetInfluenceRadius();
    const int teamId = target->GetTeam();

    for (int i = 0; i < kSampleCount; ++i) {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(kSampleCount);
        float px = center.x + radius * std::cos(angle);
        float py = center.y + radius * std::sin(angle);

        if (wrapBorders) {
            const Vector2 wrapped = ArenaGeometry::WrapPosition(px, py, arenaW, arenaH);
            px = wrapped.x;
            py = wrapped.y;
        } else if (px < 0.0f || py < 0.0f || px > arenaW || py > arenaH) {
            continue;
        }

        bool walled = false;
        for (const Obstacle& obstacle : obstacles) {
            if (!obstacle.blocksInfluence) {
                continue;
            }
            const float distSq = ArenaGeometry::DistanceSq(
                px, py, obstacle.x, obstacle.y, arenaW, arenaH, wrapBorders);
            if (distSq <= obstacle.radius * obstacle.radius) {
                walled = true;
                break;
            }
        }
        if (walled) {
            continue;
        }

        bool inEnemy = false;
        for (const IGameUnit* unit : allUnits) {
            if (!unit || unit == target || unit->GetTeam() == teamId) {
                continue;
            }
            if (InsideDisc(px, py, unit, arenaW, arenaH, wrapBorders)) {
                inEnemy = true;
                break;
            }
        }
        if (!inEnemy) {
            return false;
        }

        for (const IGameUnit* unit : allUnits) {
            if (!unit || unit == target || unit->GetTeam() != teamId) {
                continue;
            }
            if (InsideDisc(px, py, unit, arenaW, arenaH, wrapBorders)) {
                return false;
            }
        }
    }

    return true;
}
