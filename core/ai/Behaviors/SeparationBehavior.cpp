#include "SeparationBehavior.h"
#include "../../ai/BehaviorContext.h"
#include "../../interfaces/IGameUnit.h"

SeparationBehavior::SeparationBehavior()
{
    params_["separationStrength"] = 100.0f;
    params_["separationRadius"]   = 50.0f;
}

Vector2 SeparationBehavior::CalculateAcceleration(const BehaviorContext& ctx)
{
    if (!ctx.self) return Vector2::Zero();

    Vector2 pos = ctx.self->GetPosition();
    Vector2 separationForce = Vector2::Zero();
    int count = 0;

    for (IGameUnit* ally : ctx.nearbyAllies) {
        if (ally == ctx.self) continue;
        Vector2 toAlly = ally->GetPosition() - pos;
        float dist = toAlly.Length();
        if (dist < params_["separationRadius"] && dist > 0.01f) {
            float strength = (params_["separationRadius"] - dist) / params_["separationRadius"];
            separationForce -= toAlly.Normalized() * strength;
            ++count;
        }
    }

    if (count > 0) {
        separationForce = separationForce / static_cast<float>(count);
        separationForce.Normalize();
        Vector2 desired = separationForce * ctx.self->GetMaxSpeed();
        return (desired - ctx.self->GetVelocity()) * params_["separationStrength"] * ctx.deltaTime;
    }

    return Vector2::Zero();
}
