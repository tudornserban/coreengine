#pragma once

#include <vector>
#include <string>
#include "core/math/Vector2.h"

class IGameUnit;

class CommandSystem {
public:
    // State transitions
    void SetUnitState(IGameUnit* unit, const std::string& stateName);
    void SetUnitsState(const std::vector<IGameUnit*>& units, const std::string& stateName);

    // Debug commands
    void MoveUnitsTo(const std::vector<IGameUnit*>& units, Vector2 destination);

    // Parameter adjustment
    void SetBehaviorParameter(IGameUnit* unit, const std::string& paramName, float value);
};

