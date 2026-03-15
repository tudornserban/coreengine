#pragma once

// Forward declaration
class RTSGameState;

/// Read-only renderer interface.
/// Graphics systems implement this to read game state without mutating it.
/// Decouples the render loop from the fixed-timestep logic loop.
class IGameLogicRenderer {
public:
    virtual ~IGameLogicRenderer() = default;

    /// Returns a snapshot of the current game state for rendering.
    /// Must be called from the render thread; game state pointer is valid
    /// until the next logic tick.
    virtual const RTSGameState* GetCurrentGameState() const = 0;
};
