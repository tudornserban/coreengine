#pragma once

// Requires SDL2. Include SDL.h before this header.
// This adapter implements IInputReader for SDL2 input handling.

#include "../../core/interfaces/IInputReader.h"

/// SDL2 input reader.
/// Wraps SDL events into the IInputReader interface.
/// See VenusWars VenusWarsPrototype/src/Platform/SDLInput.h for reference.
class SDLInputReader : public IInputReader {
private:
    bool running_   = true;
    Vector2 mousePos_;
    // SDL key/mouse state arrays — populated by PollEvents()

public:
    void PollEvents() override;
    bool IsRunning() const override { return running_; }

    Vector2 GetMousePosition() const override { return mousePos_; }
    bool IsMouseButtonDown(int button) const override;
    bool IsMouseButtonPressed(int button) const override;
    bool IsMouseButtonReleased(int button) const override;

    bool IsKeyDown(int keyCode) const override;
    bool IsKeyPressed(int keyCode) const override;
    bool IsKeyReleased(int keyCode) const override;
};
