#include "GameLogicEngine.h"
#include <algorithm>

GameLogicEngine::GameLogicEngine(float fixedTimestep)
    : fixedTimestep_(fixedTimestep)
    , accumulatedTime_(0.0f)
    , gameTime_(0.0f)
    , isPaused_(false)
    , speedMultiplier_(1.0f)
    , fastForward_(false) {
}

void GameLogicEngine::SetUpdateCallback(std::function<void(float)> callback) {
    updateCallback_ = callback;
}

void GameLogicEngine::SetShouldEndCallback(std::function<bool()> callback) {
    shouldEndCallback_ = callback;
}

bool GameLogicEngine::Update(float deltaTime) {
    if (isPaused_ && !fastForward_) {
        return true;  // Paused, but don't exit
    }
    
    // Apply speed multiplier
    float adjustedDelta = deltaTime * speedMultiplier_;
    
    if (fastForward_) {
        // Fast-forward mode: run as fast as possible
        // Run multiple fixed timesteps per frame
        while (true) {
            if (shouldEndCallback_ && shouldEndCallback_()) {
                fastForward_ = false;
                return false;  // End condition met, stop fast-forward
            }
            
            if (updateCallback_) {
                updateCallback_(fixedTimestep_);
            }
            
            gameTime_ += fixedTimestep_;
            
            // Limit iterations per frame to prevent blocking
            if (gameTime_ > accumulatedTime_ + 1.0f) {
                break;  // Processed 1 second, yield to graphics
            }
        }
        accumulatedTime_ = gameTime_;
        return true;
    }
    
    // Normal fixed timestep logic
    accumulatedTime_ += adjustedDelta;
    
    while (accumulatedTime_ >= fixedTimestep_) {
        if (updateCallback_) {
            updateCallback_(fixedTimestep_);
        }
        
        gameTime_ += fixedTimestep_;
        accumulatedTime_ -= fixedTimestep_;
    }
    
    return true;
}

void GameLogicEngine::Reset() {
    accumulatedTime_ = 0.0f;
    gameTime_ = 0.0f;
    isPaused_ = false;
    speedMultiplier_ = 1.0f;
    fastForward_ = false;
}

