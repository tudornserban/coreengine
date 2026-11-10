#pragma once

#include "Camera.h"
#include "DrawList.h"
#include "InputScript.h"
#include "PlaybackControl.h"
#include "Overlay.h"
#include "Scenario.h"
#include "ViewerInput.h"

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
    DrawList BuildViewDrawList() const;

    bool ExecuteScript(const std::vector<ScriptCommand>& commands, std::string& error);

    void OnMouseLeftDown(Vector2 screen, bool shiftHeld);
    void OnMouseLeftUp(Vector2 screen, bool shiftHeld);
    void OnMouseMotion(Vector2 screen);
    void OnMouseRightDown(Vector2 screen, bool shiftHeld);
    void OnMouseWheel(float screenX, float screenY, float wheelDelta);
    void OnMouseMiddleDown(Vector2 screen);
    void OnMouseMiddleUp(Vector2 screen);
    void OnMouseMiddleMotion(Vector2 screen);

    void ToggleOverlayFromFKey(int fKeyIndex);
    void ApplyViewerKeyAction(const ViewerKeyAction& action);
    const OverlayFlags& GetOverlayFlags() const { return overlayFlags_; }
    int CountFiredPendingScenarioEvents() const;

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
    std::vector<ScenarioEvent> pendingEvents_;
    Camera camera_;
    bool middleMouseDown_ = false;
    Vector2 middleDragLastScreen_;

    void BindUpdateCallback();
    void FirePendingScenarioEvents();
    void RefreshCameraFromMap();
    void SubscribeCollisionRecorder();
    void UnsubscribeCollisionRecorder();
    bool ApplyScriptCommand(const ScriptCommand& command, std::string& error);

    void BeginSimulationTick();
    void EndSimulationTick();
    void RunStepSimulationTick();
};
