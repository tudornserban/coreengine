#pragma once

#include "core/interfaces/IGameUnit.h"
#include "core/math/Vector2.h"
#include "CollisionContact.h"
#include <vector>

class NarrowPhaseDetector {
public:
    // Circle-circle collision detection
    static bool DetectCircleCircle(
        IGameUnit* unitA,
        IGameUnit* unitB,
        CollisionContact& contact
    );

    // Circle-AABB collision detection (for map edges)
    static bool DetectCircleAABB(
        IGameUnit* unit,
        Vector2 aabbMin,
        Vector2 aabbMax,
        CollisionContact& contact
    );

    // Check if unit is colliding with map edge
    static bool DetectMapEdge(
        IGameUnit* unit,
        Vector2 mapMin,
        Vector2 mapMax,
        CollisionContact& contact
    );

private:
    // Helper: Calculate contact point and normal for circle-circle
    static void CalculateCircleCircleContact(
        Vector2 posA, float radiusA,
        Vector2 posB, float radiusB,
        CollisionContact& contact
    );

    // Helper: Find closest point on AABB to circle
    static Vector2 ClosestPointOnAABB(Vector2 point, Vector2 aabbMin, Vector2 aabbMax);
};

