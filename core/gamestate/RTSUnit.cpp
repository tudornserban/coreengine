#include "RTSUnit.h"
#include "../interfaces/IBehavior.h"

RTSUnit::RTSUnit(Vector2 startPos, int teamId, float unitRadius, float unitMaxSpeed)
    : position(startPos)
    , velocity(Vector2::Zero())
    , acceleration(Vector2::Zero())
    , maxSpeed(unitMaxSpeed)
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
    , stateMachine(this)
    , debugMode(false)
    , debugDestination(Vector2::Zero())
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

void RTSUnit::ApplyAcceleration(Vector2 accel, float deltaTime) {
    if (debugMode) return;

    acceleration = accel;
    velocity += acceleration * deltaTime;

    if (velocity.LengthSquared() > maxSpeed * maxSpeed) {
        velocity = velocity.Normalized() * maxSpeed;
    }

    position += velocity * deltaTime;
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
