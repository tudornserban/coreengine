#pragma once

#include "../interfaces/IGameUnit.h"
#include "../math/Vector2.h"
#include "CollisionTypes.h"

class ICollidable;

struct CollisionEvent {
    IGameUnit* agent;
    IGameUnit* otherAgent;  // nullptr for map collisions
    ICollidable* collidingObject;  // Agent or map element (for future)
    Vector2 contactPoint;
    Vector2 normal;
    float impulseMagnitude;  // Magnitude of impulse applied
    float impulseDifferential;  // Normal component of impulse vector difference
    float relativeVelocity;  // Relative velocity along normal
    CollisionType type;
    float damageModifier;  // For damage calculation

    CollisionEvent()
        : agent(nullptr)
        , otherAgent(nullptr)
        , collidingObject(nullptr)
        , contactPoint(Vector2::Zero())
        , normal(Vector2::Zero())
        , impulseMagnitude(0.0f)
        , impulseDifferential(0.0f)
        , relativeVelocity(0.0f)
        , type(CollisionType::AGENT_AGENT)
        , damageModifier(1.0f) {}
};

