#pragma once

#include "../math/Vector2.h"

/// Optional write-only kinematics surface for collision and similar systems.
/// Not part of IGameUnit; only concrete types that support direct motion writes implement this.
class IUnitKinematics {
public:
    virtual ~IUnitKinematics() = default;
    virtual void SetPosition(Vector2 pos) = 0;
    virtual void SetVelocity(Vector2 vel) = 0;
};
