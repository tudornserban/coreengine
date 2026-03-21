#pragma once

#include "../math/Vector2.h"

/// Abstract input interface.
/// Implement for SDL2, Raylib, UE5 input, network-driven commands, etc.
/// The core engine only queries this interface — never a platform SDK directly.
class IInputReader {
public:
    virtual ~IInputReader() = default;

    /// Poll and process platform-specific input events this frame.
    virtual void PollEvents() = 0;

    /// Returns true while the application should keep running.
    virtual bool IsRunning() const = 0;

    // --- Mouse / Pointer ---
    virtual Vector2 GetMousePosition() const = 0;
    virtual bool IsMouseButtonDown(int button) const = 0;
    virtual bool IsMouseButtonPressed(int button) const = 0;
    virtual bool IsMouseButtonReleased(int button) const = 0;

    // --- Keyboard ---
    virtual bool IsKeyDown(int keyCode) const = 0;
    virtual bool IsKeyPressed(int keyCode) const = 0;
    virtual bool IsKeyReleased(int keyCode) const = 0;

    // --- Gamepad (optional; default no-op) ---
    virtual bool IsGamepadButtonDown(int /*gamepad*/, int /*button*/) const { return false; }
    virtual float GetGamepadAxis(int /*gamepad*/, int /*axis*/) const { return 0.0f; }
};
