#pragma once

#include "core/interfaces/IGameUnit.h"
#include "core/math/Vector2.h"
#include "CollisionContact.h"

class MapCollisionHandler {
public:
    // Detect and resolve map edge collisions
    static bool DetectAndResolveMapEdge(
        IGameUnit* unit,
        Vector2 mapMin,
        Vector2 mapMax,
        CollisionContact& contact,
        float mapRestitution,
        float mass
    );

    // Push unit back inside map bounds
    static void PushInsideBounds(
        IGameUnit* unit,
        Vector2 mapMin,
        Vector2 mapMax
    );
};

