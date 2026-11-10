#include "Camera.h"
#include "FrameLoop.h"
#include "InputScript.h"
#include "Scenario.h"

#include "core/interfaces/IGameUnit.h"

#include <gtest/gtest.h>

TEST(CommandScriptTest, ClickOnUnitThenRightClickMovesItCloser)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    Vector2 screen = camera.WorldToScreen(Vector2(200.0f, 300.0f));
    ApplyClick(*scene.state, camera, screen);
    ASSERT_TRUE(scene.state->GetAllUnits()[0]->IsSelected());
    EXPECT_FALSE(scene.state->GetAllUnits()[1]->IsSelected());
    Vector2 destScreen = camera.WorldToScreen(Vector2(400.0f, 300.0f));
    ApplyRightClick(*scene.state, camera, destScreen);
    EXPECT_TRUE(scene.state->GetAllUnits()[0]->IsInDebugMode());
    EXPECT_FALSE(scene.state->GetAllUnits()[1]->IsInDebugMode());
    scene.state->Update(1.0f);
    EXPECT_GT(scene.state->GetAllUnits()[0]->GetPosition().x, 250.0f);
}

TEST(CommandScriptTest, BoxSelectsBothUnits)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    Vector2 a = camera.WorldToScreen(Vector2(100.0f, 200.0f));
    Vector2 b = camera.WorldToScreen(Vector2(700.0f, 400.0f));
    ApplyBox(*scene.state, camera, a, b);
    EXPECT_TRUE(scene.state->GetAllUnits()[0]->IsSelected());
    EXPECT_TRUE(scene.state->GetAllUnits()[1]->IsSelected());
}

TEST(CommandScriptTest, BoxSelectsBothUnitsWithReversedCorners)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    Vector2 a = camera.WorldToScreen(Vector2(700.0f, 400.0f));
    Vector2 b = camera.WorldToScreen(Vector2(100.0f, 200.0f));
    ApplyBox(*scene.state, camera, a, b);
    EXPECT_TRUE(scene.state->GetAllUnits()[0]->IsSelected());
    EXPECT_TRUE(scene.state->GetAllUnits()[1]->IsSelected());
}

TEST(CommandScriptTest, RightClickWithoutSelectionIsNoOp)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    Vector2 destScreen = camera.WorldToScreen(Vector2(400.0f, 300.0f));
    ApplyRightClick(*scene.state, camera, destScreen);
    EXPECT_FALSE(scene.state->GetAllUnits()[0]->IsInDebugMode());
    EXPECT_FALSE(scene.state->GetAllUnits()[1]->IsInDebugMode());
}

TEST(CommandScriptTest, ClickMissesUnitOutsideHitRadius)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
  // Team-1 unit at world (200,300), radius 12, tolerance 15 => hit radius 27.
    Vector2 nearMissWorld(200.0f + 12.0f + 15.0f + 2.0f, 300.0f);
    Vector2 screen = camera.WorldToScreen(nearMissWorld);
    ApplyClick(*scene.state, camera, screen);
    EXPECT_FALSE(scene.state->GetAllUnits()[0]->IsSelected());
    EXPECT_FALSE(scene.state->GetAllUnits()[1]->IsSelected());
}

TEST(CommandScriptTest, ClickHitsUnitAtRadiusBoundary)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    Vector2 boundaryWorld(200.0f + 12.0f + 15.0f - 0.5f, 300.0f);
    Vector2 screen = camera.WorldToScreen(boundaryWorld);
    ApplyClick(*scene.state, camera, screen);
    EXPECT_TRUE(scene.state->GetAllUnits()[0]->IsSelected());
}

TEST(CommandScriptTest, FrameLoopScriptClickRclickAndFrames)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    std::string error;
    auto cmds = ParseScript("click 427 360\nrclick 640 360\nframes 30\n", error);
    ASSERT_TRUE(error.empty());
    ASSERT_TRUE(loop.ExecuteScript(cmds, error));
    ASSERT_TRUE(error.empty());
    EXPECT_TRUE(loop.State().GetAllUnits()[0]->IsSelected());
    EXPECT_GT(loop.State().GetAllUnits()[0]->GetPosition().x, 220.0f);
}

TEST(CommandScriptTest, ScreenWorldMappingRequiredForClick)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    Vector2 screen = camera.WorldToScreen(Vector2(200.0f, 300.0f));
    ApplyClick(*scene.state, camera, Vector2(200.0f, 300.0f));
    EXPECT_FALSE(scene.state->GetAllUnits()[0]->IsSelected());
    ApplyClick(*scene.state, camera, screen);
    EXPECT_TRUE(scene.state->GetAllUnits()[0]->IsSelected());
}
