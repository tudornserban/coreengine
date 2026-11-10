#include "core/gamestate/RTSGameState.h"

#include "DrawList.h"
#include "FrameLoop.h"
#include "InputScript.h"
#include "PlaybackControl.h"
#include "Scenario.h"

#include <gtest/gtest.h>

TEST(PlaybackControlTest, StepWhilePausedRunsTheCallbackOnceAndAdvancesGameTime)
{
    int calls = 0;
    GameLogicEngine engine(kFixedDt);
    engine.SetUpdateCallback([&](float dt) {
        EXPECT_FLOAT_EQ(dt, kFixedDt);
        ++calls;
    });
    engine.SetPaused(true);
    engine.SetSpeedMultiplier(4.0f);
    TickResult result = StepOneTick(engine);
    EXPECT_EQ(result.stepsRun, 1);
    EXPECT_EQ(calls, 1);
    EXPECT_TRUE(engine.IsPaused());
    EXPECT_FLOAT_EQ(engine.GetSpeedMultiplier(), 4.0f);
    EXPECT_FLOAT_EQ(engine.GetGameTime(), kFixedDt);
}

TEST(PlaybackControlTest, StepWhileRunningDoesNothing)
{
    int calls = 0;
    GameLogicEngine engine(kFixedDt);
    engine.SetUpdateCallback([&](float) { ++calls; });
    EXPECT_EQ(StepOneTick(engine).stepsRun, 0);
    EXPECT_EQ(calls, 0);
}

TEST(PlaybackControlTest, SpaceIsPauseAndPeriodIsStep)
{
    EXPECT_EQ(CommandForKey(ViewerKey::Space), PlaybackCommand::TogglePause);
    EXPECT_EQ(CommandForKey(ViewerKey::Period), PlaybackCommand::StepOnce);
    EXPECT_EQ(CommandForKey(ViewerKey::Digit3), PlaybackCommand::DoubleSpeed);
}

TEST(PlaybackControlTest, UnknownKeyMapsToNoPlaybackCommand)
{
    EXPECT_EQ(CommandForKey(ViewerKey::Unknown), PlaybackCommand::None);
}

TEST(PlaybackControlTest, ResetRestoresTheStartPosition)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    IGameUnit* unit = scene.state->GetAllUnits()[0];
    unit->SetDebugDestination(Vector2(400.0f, 300.0f));
    scene.state->Update(kFixedDt);
    EXPECT_GT(unit->GetPosition().x, 200.0f);
    scene = BuildPreset(Preset::TwoUnits, 1);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[0]->GetPosition().x, 200.0f);
}

TEST(PlaybackControlTest, ScriptStepMovesUnitWhilePausedFramesDoNot)
{
    std::string error;
    {
        FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
        ASSERT_TRUE(loop.ExecuteScript(ParseScript("pause\ndest 0 400 300\nframes 2\n", error), error));
        const float pausedX = loop.State().GetAllUnits()[0]->GetPosition().x;
        EXPECT_FLOAT_EQ(pausedX, 200.0f);
    }
    {
        FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
        ASSERT_TRUE(loop.ExecuteScript(
            ParseScript("pause\ndest 0 400 300\nstep\nframes 1\n", error), error));
        const float steppedX = loop.State().GetAllUnits()[0]->GetPosition().x;
        EXPECT_GT(steppedX, 200.0f);
        EXPECT_LT(steppedX, 210.0f);
    }
}

TEST(PlaybackControlTest, StepWithDestinationMovesUnitOneTick)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    GameLogicEngine engine(kFixedDt);
    engine.SetUpdateCallback([&](float dt) { scene.state->Update(dt); });
    scene.state->GetAllUnits()[0]->SetDebugDestination(Vector2(400.0f, 300.0f));
    engine.SetPaused(true);
    StepOneTick(engine);
    const float x = scene.state->GetAllUnits()[0]->GetPosition().x;
    EXPECT_GT(x, 200.0f);
    EXPECT_LT(x, 210.0f);
}
