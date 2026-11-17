#include "TacticalFrontlineBehavior.h"

#include "core/ai/BehaviorContext.h"
#include "core/interfaces/IGameUnit.h"
#include "core/math/ArenaGeometry.h"

namespace {

Vector2 OffsetTo(const IGameUnit* self, const IGameUnit* other, const BehaviorContext& ctx, bool wrap) {
    const Vector2 a = self->GetPosition();
    const Vector2 b = other->GetPosition();
    if (!wrap) {
        return b - a;
    }
    return ArenaGeometry::Delta(a.x, a.y, b.x, b.y, ctx.mapSize.x, ctx.mapSize.y, true);
}

void AccumulateAway(Vector2& steer, const Vector2& toward, float radius, float strength) {
    const float dist = toward.Length();
    if (dist <= 0.01f || dist >= radius || strength == 0.0f) {
        return;
    }
    const float weight = (radius - dist) / radius;
    steer -= toward.Normalized() * weight * strength;
}

}  // namespace

TacticalFrontlineBehavior::TacticalFrontlineBehavior() {
    params_["allyAvoidRadius"] = 80.0f;
    params_["allyAvoidStrength"] = 90.0f;
    params_["allyRepulsionRadius"] = 36.0f;
    params_["allyRepulsionStrength"] = 160.0f;
    params_["enemyAvoidRadius"] = 70.0f;
    params_["enemyAvoidStrength"] = 110.0f;
    params_["clusterAttractRadius"] = 220.0f;
    params_["clusterAttractStrength"] = 80.0f;
    params_["wrapBorders"] = 0.0f;
}

Vector2 TacticalFrontlineBehavior::CalculateAcceleration(const BehaviorContext& ctx) {
    if (!ctx.self) {
        return Vector2::Zero();
    }

    const bool wrap = params_["wrapBorders"] > 0.5f;
    Vector2 steer = Vector2::Zero();

    for (IGameUnit* ally : ctx.nearbyAllies) {
        if (!ally || ally == ctx.self) {
            continue;
        }
        const Vector2 toward = OffsetTo(ctx.self, ally, ctx, wrap);
        AccumulateAway(steer, toward, params_["allyAvoidRadius"], params_["allyAvoidStrength"]);
        AccumulateAway(steer, toward, params_["allyRepulsionRadius"], params_["allyRepulsionStrength"]);
    }

    Vector2 cluster = Vector2::Zero();
    int clusterCount = 0;
    for (IGameUnit* enemy : ctx.nearbyEnemies) {
        if (!enemy || enemy == ctx.self) {
            continue;
        }
        const Vector2 toward = OffsetTo(ctx.self, enemy, ctx, wrap);
        AccumulateAway(steer, toward, params_["enemyAvoidRadius"], params_["enemyAvoidStrength"]);
        const float dist = toward.Length();
        if (dist > 0.01f && dist <= params_["clusterAttractRadius"]) {
            cluster += toward.Normalized();
            ++clusterCount;
        }
    }

    if (clusterCount > 0 && params_["clusterAttractStrength"] != 0.0f) {
        steer += cluster.Normalized() * params_["clusterAttractStrength"];
    }

    return (steer - ctx.self->GetVelocity()) * ctx.deltaTime;
}
