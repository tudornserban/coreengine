#pragma once

// Requires SDL2. Include SDL.h before this header.
// This adapter implements IGameLogicRenderer for SDL2 rendering.
// Moved from VenusWars VenusWarsPrototype/src/Platform/SDLRenderer.h

#include "../../core/interfaces/IGameLogicRenderer.h"

// Forward declarations for SDL types (avoids SDL.h dependency in this header)
struct SDL_Renderer;
struct SDL_Window;

/// SDL2 game logic renderer.
/// Reads RTSGameState and draws units, influence fields, and map boundaries.
/// See VenusWars VenusWarsPrototype/src/Platform/SDLRenderer.h for the full
/// reference implementation — copy and adapt as needed.
class SDLRenderer : public IGameLogicRenderer {
private:
    const RTSGameState* state_ = nullptr;
    SDL_Renderer* renderer_    = nullptr;
    SDL_Window* window_        = nullptr;

public:
    SDLRenderer(SDL_Renderer* renderer, SDL_Window* window)
        : renderer_(renderer), window_(window) {}

    void SetGameState(const RTSGameState* state) { state_ = state; }
    const RTSGameState* GetCurrentGameState() const override { return state_; }

    /// Render the current game state using SDL2 primitives.
    /// Implement by referencing VenusWars SDLRenderer.cpp.
    void Render();
};
