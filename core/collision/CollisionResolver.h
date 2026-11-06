#pragma once

#include "CollisionContact.h"
#include "CollisionPhysics.h"
#include "../interfaces/IGameUnit.h"

class CollisionResolver {
public:
    // Resolve collision between two agents
    static void ResolveAgentAgent(
        IGameUnit* unitA,
        IGameUnit* unitB,
        const CollisionContact& contact,
        float restitutionA,
        float restitutionB,
        float massA,
        float massB
    );

    // Resolve collision between agent and map
    static void ResolveAgentMap(
        IGameUnit* unit,
        const CollisionContact& contact,
        float mapRestitution,
        float mass
    );

    // Calculate impulse magnitude
    static float CalculateImpulseMagnitude(
        Vector2 relativeVelocity,
        Vector2 normal,
        float restitution,
        float massA,
        float massB
    );

    // Apply impulse to unit
    static void ApplyImpulse(IGameUnit* unit, Vector2 impulse, float mass);
};

