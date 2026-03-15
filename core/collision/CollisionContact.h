#pragma once

#include "../math/Vector2.h"
#include "CollisionTypes.h"

class IGameUnit;

struct CollisionContact {
    IGameUnit* unitA;
    IGameUnit* unitB;  // nullptr for map collisions
    Vector2 contactPoint;
    Vector2 normal;  // Normal pointing from B to A
    float penetration;  // Overlap depth
    CollisionType type;  // AGENT_AGENT, AGENT_MAP_EDGE, etc.

    CollisionContact()
        : unitA(nullptr)
        , unitB(nullptr)
        , contactPoint(Vector2::Zero())
        , normal(Vector2::Zero())
        , penetration(0.0f)
        , type(CollisionType::AGENT_AGENT) {}
};

