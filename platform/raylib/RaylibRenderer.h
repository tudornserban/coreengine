#pragma once

// Requires Raylib. Include raylib.h before this header.
// This adapter implements IGameLogicRenderer for Raylib rendering.
// Moved and generalized from DynamicGO's Renderer.h

#include "../../core/interfaces/IGameLogicRenderer.h"

/// Raylib game logic renderer.
/// Reads RTSGameState and draws units, influence fields, and map boundaries.
/// See DynamicGO/src/Renderer.h for the reference implementation.
class RaylibRenderer : public IGameLogicRenderer {
private:
    const RTSGameState* state_ = nullptr;
    int screenWidth_  = 1600;
    int screenHeight_ = 900;

public:
    RaylibRenderer(int w = 1600, int h = 900)
        : screenWidth_(w), screenHeight_(h) {}

    void SetGameState(const RTSGameState* state) { state_ = state; }
    const RTSGameState* GetCurrentGameState() const override { return state_; }

    /// Render using Raylib draw calls (implement in .cpp).
    void Render();
};
