#pragma once

#include "core/gameloop/GameLogicEngine.h"

enum class ViewerKey
{
    Space,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Period,
    R,
    Unknown
};

enum class PlaybackCommand
{
    None,
    TogglePause,
    HalfSpeed,
    NormalSpeed,
    DoubleSpeed,
    QuadSpeed,
    StepOnce,
    ResetScene
};

PlaybackCommand CommandForKey(ViewerKey key);

struct TickResult
{
    int stepsRun = 0;
};

TickResult StepOneTick(GameLogicEngine& engine);
