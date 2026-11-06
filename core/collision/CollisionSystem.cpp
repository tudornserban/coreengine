#include "CollisionSystem.h"
#include "../interfaces/IGameUnit.h"
#include "../interfaces/IUnitKinematics.h"
#include "ICollisionDetector.h"  // For MapBounds
#include "../logging/Logger.h"
#include <algorithm>
#include <cmath>

CollisionSystem::CollisionSystem()
    : detector(std::make_unique<CollisionDetector>())
    , immunityManager(settings.immunityFrames) {
}

CollisionSystem::CollisionSystem(const CollisionSettings& customSettings)
    : settings(customSettings)
    , detector(std::make_unique<CollisionDetector>())
    , immunityManager(settings.immunityFrames) {
}

void CollisionSystem::ProcessCollisions(
    const std::vector<IGameUnit*>& units,
    const MapBounds& bounds) {

    if (units.empty()) return;

    // Store current bounds for use in ResolveCollision
    currentBounds = bounds;

    // Detect collisions
    std::vector<CollisionContact> contacts = detector->DetectCollisions(units, bounds);

    // Resolve collisions
    for (const auto& contact : contacts) {
        ResolveCollision(contact);
    }
}

void CollisionSystem::ResolveCollision(const CollisionContact& contact) {
    if (!contact.unitA) return;

    IGameUnit* unitA = contact.unitA;

    if (contact.type == CollisionType::AGENT_AGENT && contact.unitB) {
        // Agent-Agent collision: require kinematics on both sides (same as pre-R1.2 RTSUnit gate).
        if (!dynamic_cast<IUnitKinematics*>(contact.unitA)) return;
        if (!dynamic_cast<IUnitKinematics*>(contact.unitB)) return;

        IGameUnit* unitB = contact.unitB;

        float massA = unitA->CalculateMass();
        float restitutionA = unitA->GetRestitution();
        float positionCorrectionA = unitA->GetPositionCorrection();

        float massB = unitB->CalculateMass();
        float restitutionB = unitB->GetRestitution();
        float positionCorrectionB = unitB->GetPositionCorrection();

        // Calculate impulse magnitude
        float impulseMagnitude = CalculateImpulseMagnitude(unitA, unitB, contact);

        // Resolve collision
        CollisionResolver::ResolveAgentAgent(
            unitA, unitB, contact,
            restitutionA, restitutionB,
            massA, massB
        );

        // Position correction
        float avgCorrection = (positionCorrectionA + positionCorrectionB) * 0.5f;
        if (avgCorrection > 0.0f) {
            PositionCorrector::CorrectAgentAgent(
                unitA, unitB, contact,
                avgCorrection, massA, massB
            );
        }

        // Calculate impulse differential (normal component of impulse vector difference)
        Vector2 velocityA = unitA->GetVelocity();
        Vector2 velocityB = unitB->GetVelocity();
        Vector2 relativeVelocity = velocityA - velocityB;
        float impulseDifferential = relativeVelocity.Dot(contact.normal);

        // Create and emit event (if not immune)
        if (immunityManager.ShouldGenerateEvent(unitA, unitB)) {
            CollisionEvent event = CreateCollisionEvent(contact, impulseMagnitude, impulseDifferential);
            eventManager.EmitCollision(event);

            // Set immunity
            immunityManager.SetImmunity(unitA, unitB, settings.immunityFrames);
        }

    } else if (contact.type == CollisionType::AGENT_MAP_EDGE) {
        // Agent-Map collision: always emit an event; kinematics gate applies only to writes.
        float massA = unitA->CalculateMass();
        float positionCorrectionA = unitA->GetPositionCorrection();

        float impulseMagnitude = CalculateImpulseMagnitude(unitA, nullptr, contact);

        if (dynamic_cast<IUnitKinematics*>(unitA)) {
            CollisionResolver::ResolveAgentMap(
                unitA, contact,
                settings.mapRestitution,
                massA
            );

            if (positionCorrectionA > 0.0f) {
                PositionCorrector::CorrectAgentMap(unitA, contact, positionCorrectionA);
            }

            MapCollisionHandler::PushInsideBounds(
                unitA,
                currentBounds.min,
                currentBounds.max
            );
        }

        CollisionEvent event = CreateCollisionEvent(contact, impulseMagnitude, 0.0f);
        eventManager.EmitCollision(event);
    }
}

CollisionEvent CollisionSystem::CreateCollisionEvent(
    const CollisionContact& contact,
    float impulseMagnitude,
    float impulseDifferential) {

    CollisionEvent event;
    event.agent = contact.unitA;
    event.otherAgent = contact.unitB;
    event.contactPoint = contact.contactPoint;
    event.normal = contact.normal;
    event.impulseMagnitude = impulseMagnitude;
    event.impulseDifferential = impulseDifferential;
    event.type = contact.type;

    // Calculate relative velocity
    if (contact.unitB) {
        Vector2 velocityA = contact.unitA->GetVelocity();
        Vector2 velocityB = contact.unitB->GetVelocity();
        Vector2 relativeVelocity = velocityA - velocityB;
        event.relativeVelocity = relativeVelocity.Dot(contact.normal);
    } else {
        event.relativeVelocity = contact.unitA->GetVelocity().Dot(contact.normal);
    }

    // Get damage modifier (via IGameUnit contract)
    event.damageModifier = contact.unitA->GetDamageModifier();

    return event;
}

float CollisionSystem::CalculateImpulseMagnitude(
    IGameUnit* unitA,
    IGameUnit* unitB,
    const CollisionContact& contact) {

    float massA = unitA->CalculateMass();
    float restitutionA = unitA->GetRestitution();

    if (unitB) {
        // Agent-Agent collision
        float massB = unitB->CalculateMass();
        float restitutionB = unitB->GetRestitution();
        float restitution = (restitutionA + restitutionB) * 0.5f;

        Vector2 relativeVelocity = unitA->GetVelocity() - unitB->GetVelocity();
        return CollisionResolver::CalculateImpulseMagnitude(
            relativeVelocity, contact.normal, restitution, massA, massB
        );
    } else {
        // Agent-Map collision
        Vector2 velocity = unitA->GetVelocity();
        float normalVelocity = velocity.Dot(contact.normal);

        if (normalVelocity > 0.0f) {
            return normalVelocity * (1.0f + settings.mapRestitution) * massA;
        }
        return 0.0f;
    }
}

void CollisionSystem::Subscribe(ICollisionObserver* observer) {
    LOG_DEBUG("CollisionSystem::Subscribe: Subscribing observer %p", observer);
    eventManager.Subscribe(observer);
    LOG_DEBUG("CollisionSystem::Subscribe: Observer subscribed successfully");
}

void CollisionSystem::Unsubscribe(ICollisionObserver* observer) {
    eventManager.Unsubscribe(observer);
}

void CollisionSystem::UpdateImmunity() {
    immunityManager.Update();
}

