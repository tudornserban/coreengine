#include "CommandSystem.h"
#include "../gamestate/RTSUnit.h"

void CommandSystem::SetUnitState(IGameUnit* unit, const std::string& stateName) {
    RTSUnit* rtsUnit = dynamic_cast<RTSUnit*>(unit);
    if (rtsUnit) {
        rtsUnit->GetStateMachine().SetState(stateName);
    }
}

void CommandSystem::SetUnitsState(const std::vector<IGameUnit*>& units, const std::string& stateName) {
    for (IGameUnit* unit : units) {
        SetUnitState(unit, stateName);
    }
}

void CommandSystem::MoveUnitsTo(const std::vector<IGameUnit*>& units, Vector2 destination) {
    // Spread units around the destination point
    float offset = 0.0f;
    float offsetIncrement = 30.0f;

    for (IGameUnit* unit : units) {
        if (unit) {
            Vector2 targetPos = destination;

            // Offset each unit slightly to avoid overlap
            if (offset > 0.0f) {
                float angle = offset * 2.0f;
                targetPos.x += std::cos(angle) * offset;
                targetPos.y += std::sin(angle) * offset;
            }

            unit->SetDebugDestination(targetPos);
            offset += offsetIncrement;
        }
    }
}

void CommandSystem::SetBehaviorParameter(IGameUnit* unit, const std::string& paramName, float value) {
    RTSUnit* rtsUnit = dynamic_cast<RTSUnit*>(unit);
    if (rtsUnit) {
        IBehavior* currentBehavior = rtsUnit->GetStateMachine().GetCurrentBehavior();
        if (currentBehavior) {
            auto& params = currentBehavior->GetParameters();
            auto it = params.find(paramName);
            if (it != params.end()) {
                it->second = value;
            }
        }
    }
}

