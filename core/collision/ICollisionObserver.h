#pragma once

#include "CollisionEvent.h"

class ICollisionObserver {
public:
    virtual ~ICollisionObserver() = default;
    virtual void OnCollision(const CollisionEvent& event) = 0;
};

