#include "GameStateManager.h"
#include "../logging/Logger.h"

GameStateManager::GameStateManager()
    : currentState(nullptr)
    , currentStateType(GameStateType::MAIN_MENU) {
    LOG_DEBUG("GameStateManager: Constructor called");
}

GameStateManager::~GameStateManager() {
    if (currentState) {
        currentState->OnExit();
    }
}

void GameStateManager::SetState(std::unique_ptr<IGameState> newState, GameStateType stateType) {
    LOG_DEBUG("GameStateManager::SetState: Called with stateType=%d, newState=%p", 
              static_cast<int>(stateType), newState.get());
    
    // Exit current state
    if (currentState) {
        LOG_DEBUG("GameStateManager::SetState: Exiting current state %p", currentState.get());
        currentState->OnExit();
        LOG_DEBUG("GameStateManager::SetState: Current state exited");
    }

    // Set new state
    currentState = std::move(newState);
    currentStateType = stateType;
    LOG_DEBUG("GameStateManager::SetState: New state set: %p, type=%d", 
              currentState.get(), static_cast<int>(currentStateType));

    // Enter new state
    if (currentState) {
        LOG_DEBUG("GameStateManager::SetState: Entering new state %p", currentState.get());
        currentState->OnEnter();
        LOG_DEBUG("GameStateManager::SetState: New state entered successfully");
    } else {
        LOG_ERROR("GameStateManager::SetState: New state is null after move!");
    }
}

void GameStateManager::TransitionToState(GameStateType stateType) {
    // This method is a placeholder - actual state creation will be handled by game layer
    // For now, just update the state type if state is already set
    currentStateType = stateType;
}

void GameStateManager::Update(float deltaTime) {
    if (currentState) {
        currentState->Update(deltaTime);
    }
}

void GameStateManager::Render() {
    if (currentState) {
        currentState->Render();
    }
}

