#pragma once

#include <string>

/// Optional contract for units that expose commandable agent state (FSM + behavior parameters).
/// Combat `CommandSystem` uses this to avoid depending on concrete `RTSUnit`.
class IUnitAgent {
public:
    virtual ~IUnitAgent() = default;

    virtual void SetAgentState(const std::string& stateName) = 0;
    virtual void SetAgentParameter(const std::string& paramName, float value) = 0;
};
