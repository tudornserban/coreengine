#pragma once

#include "Camera.h"
#include "DrawList.h"
#include "InputScript.h"
#include "PlaybackControl.h"
#include "Overlay.h"
#include "Scenario.h"

#include "core/gamestate/RTSGameState.h"
#include "core/gameloop/GameLogicEngine.h"
#include "core/gameloop/PlaybackController.h"

#include <cstdint>
#include <memory>
#include <vector>

class SelectionSystem;

struct DrawList;

void ApplyClick(RTSGameState& state, const Camera& camera, Vector2 screen, bool shiftHeld = false);
void ApplyRightClick(RTSGameState& state, const Camera& camera, Vector2 screen);
void ApplyBox(RTSGameState& state, const Camera& camera, Vector2 screenA, Vector2 screenB);
void AppendSelectionBox(DrawList& list, const SelectionSystem& selection, const Camera& camera);

class FrameLoop
{
public:
    FrameLoop(Preset preset, std::uint64_t seed, int viewportW, int viewportH);
    ~FrameLoop();
    FrameLoop(const FrameLoop&) = delete;
    FrameLoop& operator=(const FrameLoop&) = delete;

    RTSGameState& State() { return *state_; }
    const RTSGameState& State() const { return *state_; }
    GameLogicEngine& Engine() { return engine_; }
    const GameLogicEngine& Engine() const { return engine_; }
    PlaybackController& Playback() { return playback_; }

    Camera GetCamera() const;

    void ResetScene();
    void ApplyPlaybackCommand(PlaybackCommand command);

    void AdvancePresentationFrame();
    void RasterizeCurrentView(Framebuffer& fb) const;

    bool ExecuteScript(const std::vector<ScriptCommand>& commands, std::string& error);

    void OnMouseLeftDown(Vector2 screen, bool shiftHeld);
    void OnMouseLeftUp(Vector2 screen, bool shiftHeld);
    void OnMouseMotion(Vector2 screen);
    void OnMouseRightDown(Vector2 screen, bool shiftHeld);

    void ToggleOverlayFromFKey(int fKeyIndex);
    const OverlayFlags& GetOverlayFlags() const { return overlayFlags_; }

private:
    Preset preset_;
    std::uint64_t seed_;
    int viewportW_;
    int viewportH_;
    std::unique_ptr<RTSGameState> state_;
    GameLogicEngine engine_;
    PlaybackController playback_;

    bool leftButtonDown_ = false;
    bool dragSelectActive_ = false;
    Vector2 dragStartScreen_;

    OverlayFlags overlayFlags_;
    CollisionRecorder collisionRecorder_;
    std::vector<ExplosionEcho> explosionEchoes_;

    void BindUpdateCallback();
    void SubscribeCollisionRecorder();
    void UnsubscribeCollisionRecorder();
    bool ApplyScriptCommand(const ScriptCommand& command, std::string& error);
};
