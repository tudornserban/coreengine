#pragma once

#include <memory>
#include <string>
#include "IGameState.h"

enum class GameStateType {
    MAIN_MENU,
    USER_PROGRESSION,
    CAMPAIGN,
    MISSION_SELECTION,
    EDIT_LOADOUT,
    MISSION,
    MISSION_END_SUMMARY
};

class GameStateManager {
private:
    std::unique_ptr<IGameState> currentState;
    GameStateType currentStateType;

public:
    GameStateManager();
    ~GameStateManager();

    // State management
    void SetState(std::unique_ptr<IGameState> newState, GameStateType stateType);
    void TransitionToState(GameStateType stateType);

    // Update current state
    void Update(float deltaTime);
    void Render();

    // Getters
    GameStateType GetCurrentStateType() const { return currentStateType; }
    IGameState* GetCurrentState() { return currentState.get(); }
    const IGameState* GetCurrentState() const { return currentState.get(); }

    // Check if a state is active
    bool IsStateActive(GameStateType stateType) const { return currentStateType == stateType; }
};

