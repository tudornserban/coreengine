#pragma once

#include "../math/Vector2.h"

enum class CollisionShape {
    CIRCLE,
    AABB,  // Axis-Aligned Bounding Box
    POLYGON  // Future
};

class ICollidable {
public:
    virtual ~ICollidable() = default;
    virtual Vector2 GetPosition() const = 0;
    virtual float GetRadius() const = 0;  // For circles
    virtual CollisionShape GetShape() const = 0;
};

