#pragma once

#include "../../core/interfaces/IGameLogicRenderer.h"

/// Null renderer — no-op implementation for headless/server/test use.
/// Always returns nullptr; call code must handle a null game state gracefully.
class NullRenderer : public IGameLogicRenderer {
private:
    const RTSGameState* state_ = nullptr;

public:
    void SetGameState(const RTSGameState* state) { state_ = state; }
    const RTSGameState* GetCurrentGameState() const override { return state_; }
};
