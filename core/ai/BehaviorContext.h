#pragma once

#include <vector>
#include "../math/Vector2.h"
#include "../influence/InfluenceFieldItem.h"

// Forward declarations
class IGameUnit;
class DeterministicRNG;

/// Context passed to every behavior tick and every state transition condition.
/// Filled by RTSGameState::BuildContextForUnit() each logic frame.
struct BehaviorContext {
    IGameUnit* self           = nullptr;
    std::vector<IGameUnit*> nearbyAllies;
    std::vector<IGameUnit*> nearbyEnemies;

    Vector2 mapCenter;
    Vector2 mapSize;
    Vector2 mapBounds;
    float   deltaTime         = 0.0f;

    /// Static influence field items (map edges, obstacles, resource attractors).
    std::vector<InfluenceFieldItem> staticFieldItems;

    /// Deterministic RNG — always use this instead of rand() for replay safety.
    DeterministicRNG* rng     = nullptr;

    BehaviorContext() = default;
};
