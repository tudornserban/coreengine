#pragma once

#include "../../core/interfaces/IInputReader.h"

/// Null input reader — all inputs return false/zero.
/// Use for headless simulation, server-side logic, and unit tests.
class NullInputReader : public IInputReader {
private:
    bool running_ = true;

public:
    void SetRunning(bool r) { running_ = r; }

    void PollEvents() override {}
    bool IsRunning() const override { return running_; }

    Vector2 GetMousePosition() const override { return Vector2::Zero(); }
    bool IsMouseButtonDown(int) const override { return false; }
    bool IsMouseButtonPressed(int) const override { return false; }
    bool IsMouseButtonReleased(int) const override { return false; }

    bool IsKeyDown(int) const override { return false; }
    bool IsKeyPressed(int) const override { return false; }
    bool IsKeyReleased(int) const override { return false; }
};
