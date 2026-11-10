#include "core/gamestate/RTSGameState.h"

#include "DrawList.h"
#include "FrameLoop.h"
#include "InputScript.h"
#include "Scenario.h"

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
