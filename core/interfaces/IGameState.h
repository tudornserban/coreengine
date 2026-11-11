#pragma once

/// Base interface for all game states (menus, missions, lobbies, etc.).
/// Managed by GameStateManager via a single active state.
class IGameState {
public:
    virtual ~IGameState() = default;

    virtual void OnEnter() = 0;
    virtual void OnExit() = 0;
    virtual void Update(float deltaTime) = 0;

    /// Rendering is optional — may be delegated to a separate renderer.
    virtual void Render() {}
};
