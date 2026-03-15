#pragma once

#include "ICollisionDetector.h"
#include "CollisionDetector.h"
#include "CollisionResolver.h"
#include "PositionCorrector.h"
#include "MapCollisionHandler.h"
#include "CollisionEventManager.h"
#include "ImmunityManager.h"
#include "CollisionSettings.h"
#include "CollisionEvent.h"
#include "ICollisionDetector.h"  // Includes MapBounds
#include "../math/Vector2.h"
#include <vector>
#include <memory>

class IGameUnit;

class CollisionSystem {
private:
    std::unique_ptr<CollisionDetector> detector;
    CollisionEventManager eventManager;
    ImmunityManager immunityManager;
    CollisionSettings settings;
    MapBounds currentBounds;  // Current map bounds for collision resolution

public:
    CollisionSystem();
    CollisionSystem(const CollisionSettings& customSettings);

    // Main collision processing
    void ProcessCollisions(
        const std::vector<IGameUnit*>& units,
        const MapBounds& bounds
    );

    // Event system
    void Subscribe(ICollisionObserver* observer);
    void Unsubscribe(ICollisionObserver* observer);

    // Immunity system
    void UpdateImmunity();  // Call each game update frame

    // Settings
    CollisionSettings& GetSettings() { return settings; }
    const CollisionSettings& GetSettings() const { return settings; }

private:
    void ResolveCollision(const CollisionContact& contact);
    CollisionEvent CreateCollisionEvent(
        const CollisionContact& contact,
        float impulseMagnitude,
        float impulseDifferential
    );
    float CalculateImpulseMagnitude(
        IGameUnit* unitA,
        IGameUnit* unitB,
        const CollisionContact& contact
    );
};

