#pragma once

#include "core/math/Vector2.h"

struct CollisionPhysics {
    float mass;  // Starts at 100, proportional to HP
    float restitution;  // Bounce coefficient (0.0-1.0)
    float mapRestitution;  // Map-specific restitution
    float positionCorrection;  // 0.0 = disabled, 1.0 = full correction
    Vector2 velocity;
    Vector2 position;
    float radius;

    CollisionPhysics()
        : mass(100.0f)
        , restitution(0.7f)
        , mapRestitution(0.8f)
        , positionCorrection(0.2f)
        , velocity(Vector2::Zero())
        , position(Vector2::Zero())
        , radius(0.0f) {}
};

