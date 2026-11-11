#include "Camera.h"
#include "DrawList.h"
#include "FrameLoop.h"
#include "InputScript.h"
#include "Scenario.h"

#include "core/combat/SelectionSystem.h"
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
    // Team-1 at (200,300), radius 12, default tolerance 15 => outer hit ring at 27 world units.
    constexpr float kOuterHit = 12.0f + 15.0f;
    Vector2 nearMissWorld(200.0f + kOuterHit + 1.0f, 300.0f);
    Vector2 screen = camera.WorldToScreen(nearMissWorld);
    ApplyClick(*scene.state, camera, screen);
    EXPECT_FALSE(scene.state->GetAllUnits()[0]->IsSelected());
    EXPECT_FALSE(scene.state->GetAllUnits()[1]->IsSelected());
}

TEST(CommandScriptTest, ClickHitsUnitAtRadiusBoundary)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    constexpr float kOuterHit = 12.0f + 15.0f;
    Vector2 boundaryWorld(200.0f + kOuterHit - 0.01f, 300.0f);
    Vector2 screen = camera.WorldToScreen(boundaryWorld);
    ApplyClick(*scene.state, camera, screen);
    EXPECT_TRUE(scene.state->GetAllUnits()[0]->IsSelected());
}

TEST(CommandScriptTest, DragSelectionBoxOutlineUsesWorldToScreenCorners)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    const Vector2 worldStart(100.0f, 200.0f);
    const Vector2 worldEnd(700.0f, 400.0f);
    SelectionSystem& selection = scene.state->GetSelectionSystem();
    selection.StartBoxSelection(worldStart);
    selection.UpdateBoxSelection(worldEnd);
    ASSERT_TRUE(selection.IsBoxSelecting());

    DrawList list;
    AppendSelectionBox(list, selection, camera);
    ASSERT_EQ(list.rects.size(), 1u);
    const DrawRect& box = list.rects[0];
    EXPECT_FALSE(box.filled);
    EXPECT_EQ(box.color.r, 255);
    EXPECT_EQ(box.color.g, 220);
    EXPECT_EQ(box.color.b, 40);
    EXPECT_EQ(box.color.a, 255);
    const Vector2 expectedMin = camera.WorldToScreen(selection.GetBoxStart());
    const Vector2 expectedMax = camera.WorldToScreen(selection.GetBoxEnd());
    EXPECT_NEAR(box.min.x, expectedMin.x, 0.01f);
    EXPECT_NEAR(box.min.y, expectedMin.y, 0.01f);
    EXPECT_NEAR(box.max.x, expectedMax.x, 0.01f);
    EXPECT_NEAR(box.max.y, expectedMax.y, 0.01f);
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
