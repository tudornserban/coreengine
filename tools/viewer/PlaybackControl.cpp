#include "PlaybackControl.h"

PlaybackCommand CommandForKey(ViewerKey key)
{
    switch (key)
    {
    case ViewerKey::Space:
        return PlaybackCommand::TogglePause;
    case ViewerKey::Digit1:
        return PlaybackCommand::HalfSpeed;
    case ViewerKey::Digit2:
        return PlaybackCommand::NormalSpeed;
    case ViewerKey::Digit3:
        return PlaybackCommand::DoubleSpeed;
    case ViewerKey::Digit4:
        return PlaybackCommand::QuadSpeed;
    case ViewerKey::Period:
        return PlaybackCommand::StepOnce;
    case ViewerKey::R:
        return PlaybackCommand::ResetScene;
    case ViewerKey::Unknown:
        return PlaybackCommand::None;
    }
    return PlaybackCommand::None;
}

TickResult StepOneTick(GameLogicEngine& engine)
{
    TickResult result;
    if (!engine.IsPaused())
    {
        return result;
    }
    const float savedSpeed = engine.GetSpeedMultiplier();
    engine.SetSpeedMultiplier(1.0f);
    engine.SetPaused(false);
    engine.Update(engine.GetFixedTimestep());
    engine.SetPaused(true);
    engine.SetSpeedMultiplier(savedSpeed);
    result.stepsRun = 1;
    return result;
}
