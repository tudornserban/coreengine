#pragma once

#include "core/interfaces/IUnitKinematics.h"
#include "tests/TestHelpers/MockGameUnit.h"

// Test double so collision gtests can exercise IUnitKinematics without RTSUnit.
class KinematicMockGameUnit : public MockGameUnit, public IUnitKinematics {
public:
    using MockGameUnit::MockGameUnit;

    void SetPosition(Vector2 pos) override { MockGameUnit::SetPosition(pos); }
    void SetVelocity(Vector2 vel) override { MockGameUnit::SetVelocity(vel); }
};
