#include "SeekBehavior.h"
#include "core/ai/BehaviorContext.h"
#include "core/interfaces/IGameUnit.h"

SeekBehavior::SeekBehavior(Vector2 target) : targetPosition_(target)
{
    params_["seekStrength"]  = 100.0f;
    params_["arrivalRadius"] = 10.0f;
    params_["slowingRadius"] = 100.0f;
}

Vector2 SeekBehavior::CalculateAcceleration(const BehaviorContext& ctx)
{
    if (!ctx.self) return Vector2::Zero();

    Vector2 toTarget = targetPosition_ - ctx.self->GetPosition();
    float distance = toTarget.Length();

    if (distance < params_["arrivalRadius"]) return Vector2::Zero();

    Vector2 desired = toTarget.Normalized() * ctx.self->GetMaxSpeed();

    if (distance < params_["slowingRadius"]) {
        desired = desired * (distance / params_["slowingRadius"]);
    }

    return (desired - ctx.self->GetVelocity()) * params_["seekStrength"] * ctx.deltaTime;
}
