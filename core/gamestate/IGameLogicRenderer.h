#pragma once

// Forward declaration
class RTSGameState;

// Interface for reading game state (read-only)
// Graphics systems implement this to render game state
class IGameLogicRenderer {
public:
    virtual ~IGameLogicRenderer() = default;
    
    // Get current game state snapshot (read-only)
    virtual const RTSGameState* GetCurrentGameState() const = 0;
};

