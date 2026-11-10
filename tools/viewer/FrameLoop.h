#pragma once

#include "Camera.h"
#include "DrawList.h"
#include "InputScript.h"
#include "PlaybackControl.h"
#include "Scenario.h"

#include "core/gamestate/RTSGameState.h"
#include "core/gameloop/GameLogicEngine.h"
#include "core/gameloop/PlaybackController.h"

#include <cstdint>
#include <memory>
#include <vector>

class FrameLoop
{
public:
    FrameLoop(Preset preset, std::uint64_t seed, int viewportW, int viewportH);
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

private:
    Preset preset_;
    std::uint64_t seed_;
    int viewportW_;
    int viewportH_;
    std::unique_ptr<RTSGameState> state_;
    GameLogicEngine engine_;
    PlaybackController playback_;

    void BindUpdateCallback();
    bool ApplyScriptCommand(const ScriptCommand& command, std::string& error);
};
