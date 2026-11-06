#include "CohesionBehavior.h"
#include "core/ai/BehaviorContext.h"
#include "core/interfaces/IGameUnit.h"

CohesionBehavior::CohesionBehavior()
{
    params_["cohesionStrength"]  = 50.0f;
    params_["perceptionRadius"]  = 150.0f;
}

Vector2 CohesionBehavior::CalculateAcceleration(const BehaviorContext& ctx)
{
    if (!ctx.self) return Vector2::Zero();

    Vector2 pos = ctx.self->GetPosition();
    Vector2 centerOfMass = Vector2::Zero();
    int count = 0;

    for (IGameUnit* ally : ctx.nearbyAllies) {
        if (ally == ctx.self) continue;
        if (ally->GetPosition().Distance(pos) < params_["perceptionRadius"]) {
            centerOfMass += ally->GetPosition();
            ++count;
        }
    }

    if (count > 0) {
        centerOfMass = centerOfMass / static_cast<float>(count);
        Vector2 desired = (centerOfMass - pos).Normalized() * ctx.self->GetMaxSpeed();
        return (desired - ctx.self->GetVelocity()) * params_["cohesionStrength"] * ctx.deltaTime;
    }

    return Vector2::Zero();
}
