#pragma once

// Base interface for all game states
// All game states (MainMenu, Mission, etc.) inherit from this
class IGameState {
public:
    virtual ~IGameState() = default;

    // Called when entering this state
    virtual void OnEnter() = 0;

    // Called when exiting this state
    virtual void OnExit() = 0;

    // Called every frame while this state is active
    virtual void Update(float deltaTime) = 0;

    // Called every frame for rendering (optional, may be handled elsewhere)
    virtual void Render() {}
};

