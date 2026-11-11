#include "FleeBehavior.h"
#include "core/ai/BehaviorContext.h"
#include "core/interfaces/IGameUnit.h"

FleeBehavior::FleeBehavior()
{
    params_["fleeStrength"] = 150.0f;
    params_["dangerRadius"] = 200.0f;
}

Vector2 FleeBehavior::CalculateAcceleration(const BehaviorContext& ctx)
{
    if (!ctx.self) return Vector2::Zero();

    Vector2 fleeDir = Vector2::Zero();
    for (IGameUnit* enemy : ctx.nearbyEnemies) {
        Vector2 toEnemy = enemy->GetPosition() - ctx.self->GetPosition();
        float dist = toEnemy.Length();
        if (dist < params_["dangerRadius"] && dist > 0.01f) {
            float strength = (params_["dangerRadius"] - dist) / params_["dangerRadius"];
            fleeDir -= toEnemy.Normalized() * strength;
        }
    }

    if (fleeDir.LengthSquared() > 0.01f) {
        Vector2 desired = fleeDir.Normalized() * ctx.self->GetMaxSpeed();
        return (desired - ctx.self->GetVelocity()) * params_["fleeStrength"] * ctx.deltaTime;
    }

    return Vector2::Zero();
}
