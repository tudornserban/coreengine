#include "WanderBehavior.h"
#include "core/ai/BehaviorContext.h"
#include "core/interfaces/IGameUnit.h"
#include "core/math/DeterministicRNG.h"
#include "core/math/MathConstants.h"
#include <cmath>
#include <algorithm>

WanderBehavior::WanderBehavior() : wanderTarget_(Vector2(1.0f, 0.0f)), timeSinceLastChange_(0.0f)
{
    params_["wanderStrength"]      = 50.0f;
    params_["wanderRadius"]        = 50.0f;
    params_["directionChangeTime"] = 2.0f;
}

Vector2 WanderBehavior::CalculateAcceleration(const BehaviorContext& ctx)
{
    if (!ctx.self) return Vector2::Zero();

    timeSinceLastChange_ += ctx.deltaTime;
    if (timeSinceLastChange_ >= params_["directionChangeTime"]) {
        wanderTarget_ = GetRandomDirection(ctx);
        timeSinceLastChange_ = 0.0f;
    }

    Vector2 pos = ctx.self->GetPosition();
    Vector2 targetPos = pos + wanderTarget_ * params_["wanderRadius"];
    targetPos.x = std::max(0.0f, std::min(ctx.mapSize.x, targetPos.x));
    targetPos.y = std::max(0.0f, std::min(ctx.mapSize.y, targetPos.y));

    Vector2 desired = (targetPos - pos).Normalized() * ctx.self->GetMaxSpeed();
    return (desired - ctx.self->GetVelocity()) * params_["wanderStrength"] * ctx.deltaTime;
}

Vector2 WanderBehavior::GetRandomDirection(const BehaviorContext& ctx)
{
    if (!ctx.rng) return Vector2(1.0f, 0.0f);
    float angle = ctx.rng->NextFloat(0.0f, CoreEngine::Math::TWO_PI);
    return Vector2(std::cos(angle), std::sin(angle));
}
