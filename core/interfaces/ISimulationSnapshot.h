#pragma once

#include "IGameUnit.h"
#include "../math/Vector2.h"
#include <vector>

/// Read-only simulation snapshot: map dimensions and live unit pointers.
/// Implemented by `RTSGameState` for JSON serialization without widening renderer APIs.
class ISimulationSnapshot {
public:
    virtual ~ISimulationSnapshot() = default;

    virtual Vector2 GetMapSize() const = 0;
    virtual std::vector<IGameUnit*> GetAllUnits() const = 0;
};
