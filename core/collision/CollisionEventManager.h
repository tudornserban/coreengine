#pragma once

#include "ICollisionObserver.h"
#include "CollisionEvent.h"
#include <vector>
#include <algorithm>

class CollisionEventManager {
private:
    std::vector<ICollisionObserver*> observers;

public:
    void Subscribe(ICollisionObserver* observer);
    void Unsubscribe(ICollisionObserver* observer);
    void EmitCollision(const CollisionEvent& event);
    void Clear();

    size_t GetObserverCount() const { return observers.size(); }
};

