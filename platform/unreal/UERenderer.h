#pragma once

// Unreal Engine 5 renderer adapter stub.
//
// PORTING GUIDE (UE5):
// 1. Copy the entire core/ directory to your UE5 project (e.g. Source/MyGame/CoreEngine/)
// 2. Wrap RTSUnit in a UCLASS() actor (see PORTING_GUIDE.md for sample code)
// 3. Implement UERenderer to read RTSGameState and update Unreal scene components
// 4. Implement UEInputReader to translate Unreal input events to IInputReader
// 5. RTSGameState, StateMachine, InfluenceField — all compile unchanged in UE5
//
// IMPORTANT: The core/ layer has zero platform dependencies.
// Do NOT include any UE headers inside core/ files.

#include "../../core/interfaces/IGameLogicRenderer.h"

/// Unreal Engine 5 renderer stub.
/// Replace this class body with UE scene component update calls.
class UERenderer : public IGameLogicRenderer {
private:
    const RTSGameState* state_ = nullptr;

public:
    void SetGameState(const RTSGameState* state) { state_ = state; }
    const RTSGameState* GetCurrentGameState() const override { return state_; }

    // In UE5, implement Render() to iterate RTSGameState units and
    // update corresponding AActor positions/animations.
    void Render() {}
};
