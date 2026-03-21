#pragma once

#include <memory>
#include <functional>

class IGameState;

// Manages fixed-timestep logic loop independently of graphics
// Handles pause, speed control, and skip-to-end functionality
class GameLogicEngine {
private:
    float fixedTimestep_;           // Fixed timestep for logic (e.g., 1/30.0f for 30Hz)
    float accumulatedTime_;         // Accumulated time since last logic update
    float gameTime_;                // Total game time elapsed
    bool isPaused_;                 // Whether logic is paused
    float speedMultiplier_;         // Speed multiplier (1.0 = normal, 2.0 = 2x speed, etc.)
    bool fastForward_;              // Skip to end mode
    
    // Callback for logic updates
    std::function<void(float)> updateCallback_;
    
    // Callback to check if game should end
    std::function<bool()> shouldEndCallback_;

public:
    GameLogicEngine(float fixedTimestep = 1.0f / 30.0f);
    
    // Update logic with fixed timestep
    // Returns true if should continue, false if should stop (e.g., fast-forward complete)
    bool Update(float deltaTime);
    
    // Set update callback (called each fixed timestep)
    void SetUpdateCallback(std::function<void(float)> callback);
    
    // Set callback to check if game should end (for fast-forward)
    void SetShouldEndCallback(std::function<bool()> callback);
    
    // Playback controls
    void SetPaused(bool paused) { isPaused_ = paused; }
    bool IsPaused() const { return isPaused_; }
    
    void SetSpeedMultiplier(float multiplier) { speedMultiplier_ = std::max(0.0f, multiplier); }
    float GetSpeedMultiplier() const { return speedMultiplier_; }
    
    void SetFastForward(bool fastForward) { fastForward_ = fastForward; }
    bool IsFastForward() const { return fastForward_; }
    
    // Getters
    float GetGameTime() const { return gameTime_; }
    float GetFixedTimestep() const { return fixedTimestep_; }
    
    // Reset engine state
    void Reset();
};

