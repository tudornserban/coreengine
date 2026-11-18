#include "RTSUnit.h"
#include "../interfaces/IBehavior.h"
#include <cmath>

RTSUnit::RTSUnit(Vector2 startPos, int teamId, float unitRadius, float unitMaxSpeed)
    : position(startPos)
    , velocity(Vector2::Zero())
    , acceleration(Vector2::Zero())
    , maxSpeed(unitMaxSpeed)
    , maxAcceleration(300.0f)
    , radius(unitRadius)
    , team(teamId)
    , selected(false)
    , currentHP(100.0f)
    , maxHP(100.0f)
    , baseMass(100.0f)
    , massModifier(1.0f)
    , restitution(0.7f)
    , positionCorrection(0.2f)
    , damageModifier(1.0f)
    , influenceRadius(unitRadius)
    , attackRange(0.0f)
    , baseDps(0.0f)
    , attackCooldown(0.0f)
    , stateMachine(this)
    , debugMode(false)
    , debugDestination(Vector2::Zero())
    , movementParticipation(true)
    , actionParticipation(true)
    , collisionParticipation(true)
    , deathLatched(false)
    , entityId(0)
{}

void RTSUnit::Update(float deltaTime) {
    if (debugMode) {
        Vector2 toTarget = debugDestination - position;
        float distance = toTarget.Length();

        if (distance > 5.0f) {
            Vector2 desiredVelocity = toTarget.Normalized() * maxSpeed;
            acceleration = (desiredVelocity - velocity) * 5.0f;
        } else {
            acceleration = -velocity * 5.0f;
            debugMode = false;
        }

        velocity += acceleration * deltaTime;
        if (velocity.LengthSquared() > maxSpeed * maxSpeed) {
            velocity = velocity.Normalized() * maxSpeed;
        }
        position += velocity * deltaTime;
    }
}

namespace {

Vector2 FiniteOrZero(Vector2 value) {
    if (!std::isfinite(value.x) || !std::isfinite(value.y)) {
        return Vector2::Zero();
    }
    return value;
}

float PositiveMass(float mass) {
    if (!(mass > 0.0f) || !std::isfinite(mass)) {
        return 1.0f;
    }
    return mass;
}

}  // namespace

void RTSUnit::ApplyAcceleration(Vector2 accel, float deltaTime) {
    if (debugMode) return;

    ApplyPropulsion(accel, deltaTime);
    IntegrateMotion(deltaTime);
}

void RTSUnit::ApplyPropulsion(Vector2 accel, float deltaTime) {
    if (debugMode || !movementParticipation) return;

    acceleration = FiniteOrZero(accel);
    velocity += acceleration * deltaTime;
    velocity = FiniteOrZero(velocity);

    if (maxSpeed >= 0.0f && velocity.LengthSquared() > maxSpeed * maxSpeed) {
        velocity = velocity.Normalized() * maxSpeed;
    }
}

void RTSUnit::ApplyImpulse(Vector2 momentum) {
    momentum = FiniteOrZero(momentum);
    ApplyVelocityChange(momentum / PositiveMass(CalculateMass()));
}

void RTSUnit::ApplyVelocityChange(Vector2 deltaV) {
    if (debugMode || !movementParticipation) return;
    velocity += FiniteOrZero(deltaV);
    velocity = FiniteOrZero(velocity);
}

void RTSUnit::IntegrateMotion(float deltaTime) {
    if (debugMode || !movementParticipation) return;
    if (!std::isfinite(deltaTime)) return;
    position += velocity * deltaTime;
    position = FiniteOrZero(position);
}

float RTSUnit::GetVisualRadius() const {
    float visual = radius;
    if (visualRadiusCurve) {
        visual = visualRadiusCurve(currentHP, maxHP, radius);
    } else if (maxHP > 0.0f) {
        visual = radius * (currentHP / maxHP);
    }
    if (!std::isfinite(visual) || visual < 0.0f) {
        return 0.0f;
    }
    return visual;
}

bool RTSUnit::ConsumeDeathEvent() {
    if (currentHP > 0.0f) {
        deathLatched = false;
        return false;
    }
    if (deathLatched) {
        return false;
    }
    deathLatched = true;
    actionParticipation = false;
    return true;
}

void RTSUnit::SetMaxAcceleration(float limit) {
    if (!std::isfinite(limit) || limit < 0.0f) {
        return;
    }
    maxAcceleration = limit;
}

void RTSUnit::SetRadius(float collisionRadius) {
    if (!std::isfinite(collisionRadius) || collisionRadius < 0.0f) {
        radius = 0.0f;
        return;
    }
    radius = collisionRadius;
}

void RTSUnit::SetMaxHP(float newMax, MaxHpPolicy policy) {
    if (!std::isfinite(newMax) || newMax < 0.0f) {
        return;
    }
    if (policy == MaxHpPolicy::PreserveRatio && maxHP > 0.0f) {
        const float ratio = currentHP / maxHP;
        maxHP = newMax;
        currentHP = ratio * newMax;
    } else {
        maxHP = newMax;
        if (currentHP > maxHP) {
            currentHP = maxHP;
        }
    }
    if (!std::isfinite(currentHP) || currentHP < 0.0f) {
        currentHP = 0.0f;
    }
}

float RTSUnit::CalculateMass() const {
    if (maxHP <= 0.0f) return baseMass;
    float hpRatio = currentHP / maxHP;
    return baseMass * hpRatio * massModifier;
}

IBehavior* RTSUnit::GetCurrentBehavior() {
    return stateMachine.GetCurrentBehavior();
}

void RTSUnit::SetAgentState(const std::string& stateName) {
    GetStateMachine().SetState(stateName);
}

void RTSUnit::SetAgentParameter(const std::string& paramName, float value) {
    IBehavior* currentBehavior = GetStateMachine().GetCurrentBehavior();
    if (currentBehavior) {
        auto& params = currentBehavior->GetParameters();
        auto it = params.find(paramName);
        if (it != params.end()) {
            it->second = value;
        }
    }
}
