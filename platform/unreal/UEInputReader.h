#pragma once

#include "core/interfaces/IInputReader.h"
#include "core/gamestate/RTSGameState.h"
#include "platform/unreal/UECoordinateBridge.h"

#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace CoreEngine {
namespace Unreal {

/// Mouse button constants matching UE5 / standard conventions.
enum UEMouseButton : int {
    UE_MOUSE_LEFT = 0,
    UE_MOUSE_RIGHT = 1,
    UE_MOUSE_MIDDLE = 2
};

/// Standard virtual key codes used by the UE5 sandbox & default GUI shortcuts.
enum UEKeyCode : int {
    UE_KEY_SPACE = 32,
    UE_KEY_PERIOD = 46,
    UE_KEY_1 = 49,
    UE_KEY_2 = 50,
    UE_KEY_3 = 51,
    UE_KEY_4 = 52,
    UE_KEY_A = 65,
    UE_KEY_C = 67,
    UE_KEY_E = 69,
    UE_KEY_F = 70,
    UE_KEY_G = 71,
    UE_KEY_I = 73,
    UE_KEY_M = 77,
    UE_KEY_P = 80,
    UE_KEY_R = 82,
    UE_KEY_S = 83,
    UE_KEY_T = 84,
    UE_KEY_W = 87,
    UE_KEY_LSHIFT = 304,
    UE_KEY_F1 = 290,
    UE_KEY_F2 = 291,
    UE_KEY_F3 = 292,
    UE_KEY_F4 = 293,
    UE_KEY_F5 = 294,
    UE_KEY_F6 = 295
};

/// Unreal Engine 5 input adapter implementing IInputReader and routing 3D world-space
/// pointer/selection/command interactions into RTSGameState.
class UEInputReader : public IInputReader {
private:
    bool running_ = true;
    Vector2 screenMousePos_{0.0f, 0.0f};
    Vector2 worldCoreMousePos_{0.0f, 0.0f};
    bool hasValidWorldCursor_ = false;

    std::unordered_set<int> keysDown_;
    std::unordered_set<int> keysPressedThisFrame_;
    std::unordered_set<int> keysReleasedThisFrame_;
    std::unordered_set<int> pendingKeysDown_;

    std::unordered_set<int> mouseDown_;
    std::unordered_set<int> mousePressedThisFrame_;
    std::unordered_set<int> mouseReleasedThisFrame_;
    std::unordered_set<int> pendingMouseDown_;

    std::unordered_map<int, float> gamepadAxes_;
    std::unordered_set<int> gamepadButtons_;

    Vector2 leftDragStartCore_{0.0f, 0.0f};
    bool isDraggingBox_ = false;
    float dragThresholdCore_ = 12.0f;

public:
    UEInputReader() = default;
    ~UEInputReader() override = default;

    // --- IInputReader interface ---
    void PollEvents() override {
        keysPressedThisFrame_.clear();
        keysReleasedThisFrame_.clear();
        for (int k : pendingKeysDown_) {
            if (keysDown_.find(k) == keysDown_.end()) {
                keysPressedThisFrame_.insert(k);
            }
        }
        for (int k : keysDown_) {
            if (pendingKeysDown_.find(k) == pendingKeysDown_.end()) {
                keysReleasedThisFrame_.insert(k);
            }
        }
        keysDown_ = pendingKeysDown_;

        mousePressedThisFrame_.clear();
        mouseReleasedThisFrame_.clear();
        for (int b : pendingMouseDown_) {
            if (mouseDown_.find(b) == mouseDown_.end()) {
                mousePressedThisFrame_.insert(b);
            }
        }
        for (int b : mouseDown_) {
            if (pendingMouseDown_.find(b) == pendingMouseDown_.end()) {
                mouseReleasedThisFrame_.insert(b);
            }
        }
        mouseDown_ = pendingMouseDown_;
    }

    bool IsRunning() const override { return running_; }
    void SetRunning(bool running) { running_ = running; }

    Vector2 GetMousePosition() const override { return screenMousePos_; }
    Vector2 GetWorldCoreMousePosition() const { return worldCoreMousePos_; }
    bool HasValidWorldCursor() const { return hasValidWorldCursor_; }

    bool IsMouseButtonDown(int button) const override {
        return mouseDown_.find(button) != mouseDown_.end();
    }
    bool IsMouseButtonPressed(int button) const override {
        return mousePressedThisFrame_.find(button) != mousePressedThisFrame_.end();
    }
    bool IsMouseButtonReleased(int button) const override {
        return mouseReleasedThisFrame_.find(button) != mouseReleasedThisFrame_.end();
    }

    bool IsKeyDown(int keyCode) const override {
        return keysDown_.find(keyCode) != keysDown_.end();
    }
    bool IsKeyPressed(int keyCode) const override {
        return keysPressedThisFrame_.find(keyCode) != keysPressedThisFrame_.end();
    }
    bool IsKeyReleased(int keyCode) const override {
        return keysReleasedThisFrame_.find(keyCode) != keysReleasedThisFrame_.end();
    }

    bool IsGamepadButtonDown(int /*gamepad*/, int button) const override {
        return gamepadButtons_.find(button) != gamepadButtons_.end();
    }
    float GetGamepadAxis(int /*gamepad*/, int axis) const override {
        auto it = gamepadAxes_.find(axis);
        return (it != gamepadAxes_.end()) ? it->second : 0.0f;
    }

    // --- UE5 Event Injection API (called by APlayerController / Enhanced Input) ---
    void InjectScreenMousePosition(Vector2 screenPos) { screenMousePos_ = screenPos; }

    void InjectWorldCoreCursor(Vector2 corePos) {
        worldCoreMousePos_ = corePos;
        hasValidWorldCursor_ = true;
    }

    bool InjectWorldCameraRay(const UEVec3& rayOriginUE, const UEVec3& rayDirectionUE,
                              const UECoordinateBridge& bridge) {
        Vector2 hitCore;
        if (bridge.RaycastGroundToCore(rayOriginUE, rayDirectionUE, &hitCore)) {
            worldCoreMousePos_ = hitCore;
            hasValidWorldCursor_ = true;
            return true;
        }
        hasValidWorldCursor_ = false;
        return false;
    }

    void InjectMouseButtonState(int button, bool isDown) {
        if (isDown) {
            pendingMouseDown_.insert(button);
        } else {
            pendingMouseDown_.erase(button);
        }
    }

    void InjectKeyState(int keyCode, bool isDown) {
        if (isDown) {
            pendingKeysDown_.insert(keyCode);
        } else {
            pendingKeysDown_.erase(keyCode);
        }
    }

    void InjectGamepadAxis(int axis, float value) { gamepadAxes_[axis] = value; }
    void InjectGamepadButton(int button, bool isDown) {
        if (isDown) {
            gamepadButtons_.insert(button);
        } else {
            gamepadButtons_.erase(button);
        }
    }

    void SetDragThresholdCore(float threshold) {
        if (threshold >= 0.0f) {
            dragThresholdCore_ = threshold;
        }
    }

    /// Process 3D ground selection (click & box select) and right-click move commands on RTSGameState.
    /// Call once per frame after PollEvents().
    void ProcessWorldInteraction(RTSGameState& gameState, float hitToleranceCore = 18.0f) {
        if (!hasValidWorldCursor_) {
            return;
        }

        const bool shiftHeld = IsKeyDown(UE_KEY_LSHIFT);

        if (IsMouseButtonPressed(UE_MOUSE_LEFT)) {
            leftDragStartCore_ = worldCoreMousePos_;
            isDraggingBox_ = false;
        }

        if (IsMouseButtonDown(UE_MOUSE_LEFT)) {
            const float dragDist = (worldCoreMousePos_ - leftDragStartCore_).Length();
            if (!isDraggingBox_ && dragDist >= dragThresholdCore_) {
                isDraggingBox_ = true;
                gameState.GetSelectionSystem().StartBoxSelection(leftDragStartCore_);
            }
            if (isDraggingBox_) {
                gameState.GetSelectionSystem().UpdateBoxSelection(worldCoreMousePos_);
            }
        }

        if (IsMouseButtonReleased(UE_MOUSE_LEFT)) {
            if (isDraggingBox_) {
                gameState.GetSelectionSystem().UpdateBoxSelection(worldCoreMousePos_);
                gameState.GetSelectionSystem().EndBoxSelection(gameState.GetAllUnits());
                isDraggingBox_ = false;
            } else {
                IGameUnit* clicked = gameState.GetUnitAtPosition(worldCoreMousePos_, hitToleranceCore);
                if (clicked) {
                    gameState.GetSelectionSystem().SelectUnit(clicked, shiftHeld);
                } else if (!shiftHeld) {
                    gameState.GetSelectionSystem().ClearSelection();
                }
            }
        }

        if (IsMouseButtonPressed(UE_MOUSE_RIGHT)) {
            const auto& selected = gameState.GetSelectionSystem().GetSelectedUnits();
            if (!selected.empty()) {
                gameState.GetCommandSystem().MoveUnitsTo(selected, worldCoreMousePos_);
            }
        }
    }
};

} // namespace Unreal
} // namespace CoreEngine

using UEInputReader = CoreEngine::Unreal::UEInputReader;
