#include "FrameLoop.h"

#include "DrawList.h"
#include "Overlay.h"
#include "Raster.h"
#include "SnapshotRenderer.h"

#include "core/combat/CommandSystem.h"
#include "core/combat/SelectionSystem.h"
#include "core/interfaces/IGameUnit.h"

#include <cmath>

namespace
{

constexpr float kDragThresholdPx = 4.0f;

} // namespace

void ApplyClick(RTSGameState& state, const Camera& camera, Vector2 screen, bool shiftHeld)
{
    const Vector2 world = camera.ScreenToWorld(screen);
    IGameUnit* unit = state.GetUnitAtPosition(world);
    if (unit != nullptr)
    {
        state.GetSelectionSystem().SelectUnit(unit, shiftHeld);
    }
}

void ApplyRightClick(RTSGameState& state, const Camera& camera, Vector2 screen)
{
    const Vector2 world = camera.ScreenToWorld(screen);
    const std::vector<IGameUnit*>& selected = state.GetSelectionSystem().GetSelectedUnits();
    if (selected.empty())
    {
        return;
    }
    state.GetCommandSystem().MoveUnitsTo(selected, world);
}

void ApplyBox(RTSGameState& state, const Camera& camera, Vector2 screenA, Vector2 screenB)
{
    SelectionSystem& selection = state.GetSelectionSystem();
    const Vector2 worldStart = camera.ScreenToWorld(screenA);
    const Vector2 worldEnd = camera.ScreenToWorld(screenB);
    selection.StartBoxSelection(worldStart);
    selection.UpdateBoxSelection(worldEnd);
    selection.EndBoxSelection(state.GetAllUnits());
}

void AppendSelectionBox(DrawList& list, const SelectionSystem& selection, const Camera& camera)
{
    if (!selection.IsBoxSelecting())
    {
        return;
    }
    const Vector2 screenMin = camera.WorldToScreen(selection.GetBoxStart());
    const Vector2 screenMax = camera.WorldToScreen(selection.GetBoxEnd());
    DrawRect box;
    box.min = screenMin;
    box.max = screenMax;
    box.filled = false;
    box.color = {255, 220, 40, 255};
    list.rects.push_back(box);
}

FrameLoop::~FrameLoop()
{
    UnsubscribeCollisionRecorder();
}

FrameLoop::FrameLoop(Preset preset, std::uint64_t seed, int viewportW, int viewportH)
    : preset_(preset)
    , seed_(seed)
    , viewportW_(viewportW)
    , viewportH_(viewportH)
    , engine_(kFixedDt)
    , playback_(&engine_)
{
    ResetScene();
}

void FrameLoop::OnMouseWheel(float screenX, float screenY, float wheelDelta)
{
    if (wheelDelta == 0.0f)
    {
        return;
    }
    const float factor = wheelDelta > 0.0f ? 1.1f : 1.0f / 1.1f;
    camera_.ZoomAt(Vector2(screenX, screenY), factor);
}

void FrameLoop::OnMouseMiddleDown(Vector2 screen)
{
    middleMouseDown_ = true;
    middleDragLastScreen_ = screen;
}

void FrameLoop::OnMouseMiddleUp(Vector2 /*screen*/)
{
    middleMouseDown_ = false;
}

void FrameLoop::OnMouseMiddleMotion(Vector2 screen)
{
    if (!middleMouseDown_)
    {
        return;
    }
    const Vector2 delta(screen.x - middleDragLastScreen_.x, screen.y - middleDragLastScreen_.y);
    camera_.Pan(delta);
    middleDragLastScreen_ = screen;
}

Camera FrameLoop::GetCamera() const
{
    return camera_;
}

void FrameLoop::BindUpdateCallback()
{
    engine_.SetUpdateCallback([this](float dt) { state_->Update(dt); });
}

void FrameLoop::SubscribeCollisionRecorder()
{
    if (state_ != nullptr)
    {
        state_->GetCollisionSystem().Subscribe(&collisionRecorder_);
    }
}

void FrameLoop::UnsubscribeCollisionRecorder()
{
    if (state_ != nullptr)
    {
        state_->GetCollisionSystem().Unsubscribe(&collisionRecorder_);
    }
}

void FrameLoop::RefreshCameraFromMap()
{
    camera_ = FitMap(state_->GetMapSize(), viewportW_, viewportH_, kFitMarginPx);
}

void FrameLoop::ResetScene()
{
    UnsubscribeCollisionRecorder();
    explosionEchoes_.clear();
    collisionRecorder_.Clear();
    Scene scene = BuildPreset(preset_, seed_);
    state_ = std::move(scene.state);
    pendingEvents_ = PendingEvents(preset_, *state_);
    engine_.Reset();
    BindUpdateCallback();
    SubscribeCollisionRecorder();
    RefreshCameraFromMap();
}

void FrameLoop::FirePendingScenarioEvents()
{
    const float gameTime = engine_.GetGameTime();
    for (ScenarioEvent& event : pendingEvents_)
    {
        if (event.fired)
        {
            continue;
        }
        if (gameTime < event.time && gameTime + kFixedDt >= event.time)
        {
            state_->AddExplosion(event.explosion);
            event.fired = true;
        }
    }
}

void FrameLoop::ApplyPlaybackCommand(PlaybackCommand command)
{
    switch (command)
    {
    case PlaybackCommand::None:
        break;
    case PlaybackCommand::TogglePause:
        playback_.TogglePause();
        break;
    case PlaybackCommand::HalfSpeed:
        playback_.SetHalfSpeed();
        break;
    case PlaybackCommand::NormalSpeed:
        playback_.SetNormalSpeed();
        break;
    case PlaybackCommand::DoubleSpeed:
        playback_.SetDoubleSpeed();
        break;
    case PlaybackCommand::QuadSpeed:
        playback_.SetQuadSpeed();
        break;
    case PlaybackCommand::StepOnce:
        if (!engine_.IsPaused())
        {
            break;
        }
        RunStepSimulationTick();
        break;
    case PlaybackCommand::ResetScene:
        ResetScene();
        break;
    }
}

void FrameLoop::BeginSimulationTick()
{
    FirePendingScenarioEvents();
    collisionRecorder_.SetReferenceGameTime(engine_.GetGameTime());
    NoteExplosions(*state_, explosionEchoes_, kFixedDt);
}

void FrameLoop::EndSimulationTick()
{
    TickExplosionEchoes(explosionEchoes_, kFixedDt);
    collisionRecorder_.ExpireOlderThan(engine_.GetGameTime(), kCollisionOverlayKeepSeconds);
}

void FrameLoop::RunStepSimulationTick()
{
    BeginSimulationTick();
    StepOneTick(engine_);
    EndSimulationTick();
}

void FrameLoop::AdvancePresentationFrame()
{
    if (!engine_.IsPaused())
    {
        BeginSimulationTick();
        engine_.Update(kFixedDt);
        EndSimulationTick();
    }
    else
    {
        engine_.Update(kFixedDt);
    }
}

DrawList FrameLoop::BuildViewDrawList() const
{
    const Camera camera = GetCamera();
    DrawList list = BuildSnapshotDrawList(*state_, camera);
    AppendSelectionBox(list, state_->GetSelectionSystem(), camera);
    AppendOverlays(list, *state_, camera, overlayFlags_, collisionRecorder_, explosionEchoes_);
    if (overlayFlags_.collisions || overlayFlags_.perception || overlayFlags_.influence || overlayFlags_.obstacles
        || overlayFlags_.behaviorItems || overlayFlags_.explosions)
    {
        AppendStatusLine(list, engine_, playback_, overlayFlags_, state_->GetAllUnits().size());
    }
    return list;
}

void FrameLoop::RasterizeCurrentView(Framebuffer& fb) const
{
    fb.width = viewportW_;
    fb.height = viewportH_;
    Rasterize(BuildViewDrawList(), fb);
}

bool FrameLoop::ApplyScriptCommand(const ScriptCommand& command, std::string& error)
{
    switch (command.kind)
    {
    case ScriptCommand::Kind::Frames:
        for (int i = 0; i < command.intArg; ++i)
        {
            AdvancePresentationFrame();
        }
        return true;
    case ScriptCommand::Kind::Pause:
        playback_.Pause();
        return true;
    case ScriptCommand::Kind::Resume:
        playback_.Resume();
        return true;
    case ScriptCommand::Kind::Step:
        if (!engine_.IsPaused())
        {
            error = "step requires the simulation to be paused";
            return false;
        }
        RunStepSimulationTick();
        return true;
    case ScriptCommand::Kind::Speed:
        if (command.floatArg == 0.5f)
        {
            playback_.SetHalfSpeed();
        }
        else if (command.floatArg == 1.0f)
        {
            playback_.SetNormalSpeed();
        }
        else if (command.floatArg == 2.0f)
        {
            playback_.SetDoubleSpeed();
        }
        else
        {
            playback_.SetQuadSpeed();
        }
        return true;
    case ScriptCommand::Kind::Reset:
        ResetScene();
        return true;
    case ScriptCommand::Kind::Dest:
    {
        const auto& units = state_->GetAllUnits();
        if (command.intArg < 0 || static_cast<size_t>(command.intArg) >= units.size())
        {
            error = "dest index out of range";
            return false;
        }
        if (units[command.intArg] == nullptr)
        {
            error = "dest index is null";
            return false;
        }
        units[command.intArg]->SetDebugDestination(Vector2(command.x, command.y));
        return true;
    }
    case ScriptCommand::Kind::Click:
        ApplyClick(*state_, GetCamera(), Vector2(command.x, command.y), false);
        return true;
    case ScriptCommand::Kind::RightClick:
        ApplyRightClick(*state_, GetCamera(), Vector2(command.x, command.y));
        return true;
    case ScriptCommand::Kind::Box:
        ApplyBox(*state_, GetCamera(), Vector2(command.x, command.y), Vector2(command.x2, command.y2));
        return true;
    case ScriptCommand::Kind::Overlay:
    {
        const bool enabled = command.intArg != 0;
        if (!SetOverlayFlag(overlayFlags_, command.strArg, enabled, error))
        {
            return false;
        }
        return true;
    }
    case ScriptCommand::Kind::Preset:
    {
        Preset preset = Preset::TwoUnits;
        if (!ParsePresetFromString(command.strArg, preset))
        {
            error = "unknown preset name";
            return false;
        }
        preset_ = preset;
        ResetScene();
        return true;
    }
    }
    error = "internal script error";
    return false;
}

bool FrameLoop::ExecuteScript(const std::vector<ScriptCommand>& commands, std::string& error)
{
    error.clear();
    for (const ScriptCommand& command : commands)
    {
        if (!ApplyScriptCommand(command, error))
        {
            return false;
        }
    }
    return true;
}

void FrameLoop::OnMouseLeftDown(Vector2 screen, bool /*shiftHeld*/)
{
    leftButtonDown_ = true;
    dragSelectActive_ = false;
    dragStartScreen_ = screen;
}

void FrameLoop::OnMouseLeftUp(Vector2 screen, bool shiftHeld)
{
    const Camera camera = GetCamera();
    if (dragSelectActive_)
    {
        state_->GetSelectionSystem().EndBoxSelection(state_->GetAllUnits());
    }
    else if (leftButtonDown_)
    {
        ApplyClick(*state_, camera, screen, shiftHeld);
    }
    leftButtonDown_ = false;
    dragSelectActive_ = false;
}

void FrameLoop::OnMouseMotion(Vector2 screen)
{
    if (!leftButtonDown_)
    {
        return;
    }
    const float dx = screen.x - dragStartScreen_.x;
    const float dy = screen.y - dragStartScreen_.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    const Camera camera = GetCamera();
    if (!dragSelectActive_ && dist >= kDragThresholdPx)
    {
        dragSelectActive_ = true;
        state_->GetSelectionSystem().StartBoxSelection(camera.ScreenToWorld(dragStartScreen_));
    }
    if (dragSelectActive_)
    {
        state_->GetSelectionSystem().UpdateBoxSelection(camera.ScreenToWorld(screen));
    }
}

void FrameLoop::OnMouseRightDown(Vector2 screen, bool /*shiftHeld*/)
{
    ApplyRightClick(*state_, GetCamera(), screen);
}

void FrameLoop::ApplyViewerKeyAction(const ViewerKeyAction& action)
{
    if (action.kind == ViewerKeyActionKind::OverlayToggle)
    {
        ToggleOverlayFromFKey(action.overlayToggleIndex);
        return;
    }
    if (action.kind == ViewerKeyActionKind::Playback)
    {
        ApplyPlaybackCommand(action.playback);
    }
}

void FrameLoop::ToggleOverlayFromFKey(int fKeyIndex)
{
    switch (OverlayKeyFromViewerFKey(fKeyIndex))
    {
    case OverlayKey::Collisions:
        overlayFlags_.collisions = !overlayFlags_.collisions;
        break;
    case OverlayKey::Perception:
        overlayFlags_.perception = !overlayFlags_.perception;
        break;
    case OverlayKey::Influence:
        overlayFlags_.influence = !overlayFlags_.influence;
        break;
    case OverlayKey::Obstacles:
        overlayFlags_.obstacles = !overlayFlags_.obstacles;
        break;
    case OverlayKey::BehaviorItems:
        overlayFlags_.behaviorItems = !overlayFlags_.behaviorItems;
        break;
    case OverlayKey::Explosions:
        overlayFlags_.explosions = !overlayFlags_.explosions;
        break;
    case OverlayKey::Unknown:
        break;
    }
}
