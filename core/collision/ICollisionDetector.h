#pragma once

#include "../math/Vector2.h"
#include "CollisionContact.h"
#include <vector>

class IGameUnit;

struct MapBounds {
    Vector2 min;
    Vector2 max;
    Vector2 size;

    MapBounds() : min(Vector2::Zero()), max(Vector2::Zero()), size(Vector2::Zero()) {}
    MapBounds(Vector2 minPos, Vector2 maxPos)
        : min(minPos), max(maxPos), size(maxPos - minPos) {}
};

class ICollisionDetector {
public:
    virtual ~ICollisionDetector() = default;

    virtual std::vector<CollisionContact> DetectCollisions(
        const std::vector<IGameUnit*>& units,
        const MapBounds& bounds
    ) = 0;
};

