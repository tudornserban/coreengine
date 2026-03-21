#include "PlaybackController.h"
#include "GameLogicEngine.h"

PlaybackController::PlaybackController(GameLogicEngine* logicEngine)
    : logicEngine_(logicEngine) {
}

void PlaybackController::TogglePause() {
    if (logicEngine_) {
        logicEngine_->SetPaused(!logicEngine_->IsPaused());
    }
}

