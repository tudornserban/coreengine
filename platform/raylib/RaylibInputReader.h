#pragma once

// Requires Raylib. Include raylib.h before this header.
// This adapter implements IInputReader for Raylib input handling.
// Generalized from DynamicGO/src/RaylibInputReader.h

#include "../../core/interfaces/IInputReader.h"

/// Raylib input reader.
/// Wraps Raylib's IsKeyDown/IsMouseButtonPressed etc. into IInputReader.
class RaylibInputReader : public IInputReader {
private:
    bool running_ = true;

public:
    void PollEvents() override;
    bool IsRunning() const override { return running_; }

    Vector2 GetMousePosition() const override;
    bool IsMouseButtonDown(int button) const override;
    bool IsMouseButtonPressed(int button) const override;
    bool IsMouseButtonReleased(int button) const override;

    bool IsKeyDown(int keyCode) const override;
    bool IsKeyPressed(int keyCode) const override;
    bool IsKeyReleased(int keyCode) const override;
};
