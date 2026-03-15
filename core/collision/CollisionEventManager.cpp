#include "CollisionEventManager.h"

void CollisionEventManager::Subscribe(ICollisionObserver* observer) {
    if (!observer) return;

    // Check if already subscribed
    auto it = std::find(observers.begin(), observers.end(), observer);
    if (it == observers.end()) {
        observers.push_back(observer);
    }
}

void CollisionEventManager::Unsubscribe(ICollisionObserver* observer) {
    if (!observer) return;

    auto it = std::find(observers.begin(), observers.end(), observer);
    if (it != observers.end()) {
        observers.erase(it);
    }
}

void CollisionEventManager::EmitCollision(const CollisionEvent& event) {
    for (ICollisionObserver* observer : observers) {
        if (observer) {
            observer->OnCollision(event);
        }
    }
}

void CollisionEventManager::Clear() {
    observers.clear();
}

