#include "core/gamestate/RTSGameState.h"

#include "DrawList.h"
#include "FrameLoop.h"
#include "InputScript.h"
#include "Overlay.h"
#include "Scenario.h"
#include "ViewerInput.h"

#include <gtest/gtest.h>

namespace
{

void ExpectTwoUnitsPresetAndFreshEngine(const FrameLoop& loop)
{
    const auto& units = loop.State().GetAllUnits();
    ASSERT_EQ(units.size(), 2u);
    EXPECT_FLOAT_EQ(units[0]->GetPosition().x, 200.0f);
    EXPECT_FLOAT_EQ(units[0]->GetPosition().y, 300.0f);
    EXPECT_FLOAT_EQ(units[1]->GetPosition().x, 600.0f);
    EXPECT_FLOAT_EQ(units[1]->GetPosition().y, 300.0f);
    EXPECT_FALSE(loop.Engine().IsPaused());
    EXPECT_FLOAT_EQ(loop.Engine().GetSpeedMultiplier(), 1.0f);
    EXPECT_FLOAT_EQ(loop.Engine().GetGameTime(), 0.0f);
}

void RunUntilUnitsMoved(FrameLoop& loop)
{
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(
        ParseScript("resume\ndest 0 400 300\nframes 20\n", error), error));
    EXPECT_GT(loop.State().GetAllUnits()[0]->GetPosition().x, 200.0f);
}

} // namespace

TEST(FrameLoopResetTest, ResetSceneRebuildsPresetAndClearsEngineTiming)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    RunUntilUnitsMoved(loop);
    loop.Playback().SetQuadSpeed();
    loop.Playback().Pause();
    loop.AdvancePresentationFrame();
    EXPECT_TRUE(loop.Engine().IsPaused());
    EXPECT_FLOAT_EQ(loop.Engine().GetSpeedMultiplier(), 4.0f);
    EXPECT_GT(loop.Engine().GetGameTime(), 0.0f);

    loop.ResetScene();
    ExpectTwoUnitsPresetAndFreshEngine(loop);
}

TEST(FrameLoopResetTest, ScriptResetVerbRebuildsPresetAndClearsEngineTiming)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    RunUntilUnitsMoved(loop);
    loop.Playback().SetQuadSpeed();
    loop.Playback().Pause();
    loop.AdvancePresentationFrame();

    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(ParseScript("reset\n", error), error));
    ExpectTwoUnitsPresetAndFreshEngine(loop);
}

namespace
{

int CountCollisionOverlayLines(const DrawList& list)
{
    int count = 0;
    for (const DrawLine& line : list.lines)
    {
        if (line.color.r == 255 && line.color.g == 160 && line.color.b == 40
            && line.color.a == 255)
        {
            ++count;
        }
    }
    return count;
}

int CountExplosionOverlayCircles(const DrawList& list)
{
    int count = 0;
    for (const DrawCircle& circle : list.circles)
    {
        if (circle.outline.r == 255 && circle.outline.g == 80 && circle.outline.b == 40
            && circle.outline.a == 255)
        {
            ++count;
        }
    }
    return count;
}

} // namespace

TEST(FrameLoopResetTest, ResetSceneClearsCollisionAndExplosionOverlayState)
{
    FrameLoop loop(Preset::Collision, 1, 1280, 720);
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(
        ParseScript("overlay collisions 1\noverlay explosions 1\nframes 30\n", error), error));
    const DrawList beforeReset = loop.BuildViewDrawList();
    const int collisionLinesBefore = CountCollisionOverlayLines(beforeReset);
    ASSERT_GE(collisionLinesBefore, 2);

    loop.ResetScene();
    EXPECT_EQ(loop.CountFiredPendingScenarioEvents(), 0);
    EXPECT_EQ(loop.State().GetExplosions().size(), 0u);
    const DrawList afterReset = loop.BuildViewDrawList();
    EXPECT_EQ(CountCollisionOverlayLines(afterReset), 0);
    EXPECT_EQ(CountExplosionOverlayCircles(afterReset), 0);
}

TEST(FrameLoopResetTest, ResetSceneAfterCombatEchoDoesNotLeakExplosionDrawables)
{
    FrameLoop loop(Preset::Combat, 1, 1280, 720);
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(
        ParseScript("overlay explosions 1\nframes 18\n", error), error));
    EXPECT_EQ(loop.CountFiredPendingScenarioEvents(), 1);
    ASSERT_GE(CountExplosionOverlayCircles(loop.BuildViewDrawList()), 1);

    loop.ResetScene();
    EXPECT_EQ(CountExplosionOverlayCircles(loop.BuildViewDrawList()), 0);
    EXPECT_EQ(loop.CountFiredPendingScenarioEvents(), 0);
    EXPECT_EQ(loop.State().GetExplosions().size(), 0u);
}

TEST(FrameLoopResetTest, HandleViewerKeycodeResetMatchesScriptResetLeakCheck)
{
    FrameLoop loop(Preset::Combat, 1, 1280, 720);
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(ParseScript("overlay explosions 1\nframes 20\n", error), error));
    EXPECT_EQ(loop.CountFiredPendingScenarioEvents(), 1);

    const ViewerKeyAction action = HandleViewerKeycode(114);
    ASSERT_EQ(action.kind, ViewerKeyActionKind::Playback);
    loop.ApplyViewerKeyAction(action);

    EXPECT_EQ(CountExplosionOverlayCircles(loop.BuildViewDrawList()), 0);
    EXPECT_EQ(loop.CountFiredPendingScenarioEvents(), 0);
    EXPECT_FLOAT_EQ(loop.Engine().GetGameTime(), 0.0f);
}
