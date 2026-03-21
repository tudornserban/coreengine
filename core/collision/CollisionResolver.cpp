#include "CollisionResolver.h"
#include "../gamestate/RTSUnit.h"

void CollisionResolver::ResolveAgentAgent(
    IGameUnit* unitA,
    IGameUnit* unitB,
    const CollisionContact& contact,
    float restitutionA,
    float restitutionB,
    float massA,
    float massB) {

    if (!unitA || !unitB) return;

    // Use average restitution for the collision
    float restitution = (restitutionA + restitutionB) * 0.5f;

    // Calculate relative velocity
    Vector2 velocityA = unitA->GetVelocity();
    Vector2 velocityB = unitB->GetVelocity();
    Vector2 relativeVelocity = velocityA - velocityB;

    // Calculate impulse magnitude
    float impulseMagnitude = CalculateImpulseMagnitude(
        relativeVelocity,
        contact.normal,
        restitution,
        massA,
        massB
    );

    // Calculate impulse vector
    Vector2 impulse = contact.normal * impulseMagnitude;

    // Apply impulses (opposite directions)
    ApplyImpulse(unitA, impulse, massA);
    ApplyImpulse(unitB, -impulse, massB);
}

void CollisionResolver::ResolveAgentMap(
    IGameUnit* unit,
    const CollisionContact& contact,
    float mapRestitution,
    float mass) {

    if (!unit) return;

    // For map collision, we reflect the velocity
    Vector2 velocity = unit->GetVelocity();

    // Project velocity onto normal
    float normalVelocity = velocity.Dot(contact.normal);

    // Only resolve if moving towards the map
    if (normalVelocity > 0.0f) {
        // Calculate impulse to reflect velocity
        float impulseMagnitude = normalVelocity * (1.0f + mapRestitution);
        Vector2 impulse = contact.normal * impulseMagnitude;

        ApplyImpulse(unit, impulse, mass);
    }
}

float CollisionResolver::CalculateImpulseMagnitude(
    Vector2 relativeVelocity,
    Vector2 normal,
    float restitution,
    float massA,
    float massB) {

    // Relative velocity along normal
    float normalVelocity = relativeVelocity.Dot(normal);

    // Only resolve if moving towards each other
    if (normalVelocity > 0.0f) {
        return 0.0f;  // Moving apart, no collision
    }

    // Effective mass (reduced mass)
    float effectiveMass = 1.0f / (1.0f / massA + 1.0f / massB);

    // Impulse magnitude with restitution
    float impulse = -(1.0f + restitution) * normalVelocity * effectiveMass;

    return impulse;
}

void CollisionResolver::ApplyImpulse(IGameUnit* unit, Vector2 impulse, float mass) {
    if (!unit || mass <= 0.0f) return;

    // Velocity change from impulse
    Vector2 velocityChange = impulse / mass;

    // Get current velocity
    Vector2 currentVelocity = unit->GetVelocity();

    // Apply velocity change
    Vector2 newVelocity = currentVelocity + velocityChange;

    // Apply new velocity (we'll need to add SetVelocity method to IGameUnit)
    // For now, we'll use RTSUnit directly
    RTSUnit* rtsUnit = dynamic_cast<RTSUnit*>(unit);
    if (rtsUnit) {
        rtsUnit->SetVelocity(newVelocity);
    }
}

