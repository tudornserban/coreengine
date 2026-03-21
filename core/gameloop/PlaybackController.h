#pragma once

#include "GameLogicEngine.h"

// Wrapper around GameLogicEngine playback controls
// Provides simpler interface for UI/control systems
class PlaybackController {
private:
    GameLogicEngine* logicEngine_;

public:
    PlaybackController(GameLogicEngine* logicEngine);
    
    // Playback controls
    void Pause() { if (logicEngine_) logicEngine_->SetPaused(true); }
    void Resume() { if (logicEngine_) logicEngine_->SetPaused(false); }
    void TogglePause();
    bool IsPaused() const { return logicEngine_ ? logicEngine_->IsPaused() : false; }
    
    void SetSpeed(float multiplier) { if (logicEngine_) logicEngine_->SetSpeedMultiplier(multiplier); }
    float GetSpeed() const { return logicEngine_ ? logicEngine_->GetSpeedMultiplier() : 1.0f; }
    
    void SetNormalSpeed() { SetSpeed(1.0f); }
    void SetHalfSpeed() { SetSpeed(0.5f); }
    void SetDoubleSpeed() { SetSpeed(2.0f); }
    void SetQuadSpeed() { SetSpeed(4.0f); }
    
    void FastForward() { if (logicEngine_) logicEngine_->SetFastForward(true); }
    void StopFastForward() { if (logicEngine_) logicEngine_->SetFastForward(false); }
    bool IsFastForward() const { return logicEngine_ ? logicEngine_->IsFastForward() : false; }
    
    float GetGameTime() const { return logicEngine_ ? logicEngine_->GetGameTime() : 0.0f; }
};

