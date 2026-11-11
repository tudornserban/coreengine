#pragma once

#include "ISimulationSnapshot.h"

// Forward declaration
class RTSGameState;

/// Read-only renderer interface.
/// Graphics systems implement this to read game state without mutating it.
/// Decouples the render loop from the fixed-timestep logic loop.
class IGameLogicRenderer {
public:
    virtual ~IGameLogicRenderer() = default;

    /// Returns the current hub for rendering — still the renderer contract; const;
    /// callers that need the concrete RTSGameState keep using this.
    /// Must be called from the render thread; game state pointer is valid
    /// until the next logic tick.
    virtual const RTSGameState* GetCurrentGameState() const = 0;

    /// Optional read-only snapshot for serializers and future render paths.
    /// Default is nullptr; override when a non-hub snapshot is available.
    virtual const ISimulationSnapshot* GetSimulationSnapshot() const { return nullptr; }
};
