#include "core/gamestate/RTSGameState.h"

#include "Camera.h"
#include "DrawList.h"
#include "Raster.h"
#include "Scenario.h"
#include "SnapshotRenderer.h"

#include <cmath>
#include <gtest/gtest.h>

TEST(SnapshotRendererTest, TwoUnitsBecomeTeamColoredCirclesAtProjectedCenters)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    DrawList list = BuildSnapshotDrawList(*scene.state, camera);
    ASSERT_EQ(list.circles.size(), 2u);
    auto center = camera.WorldToScreen(Vector2(200.0f, 300.0f));
    EXPECT_NEAR(list.circles[0].center.x, center.x, 0.01f);
    EXPECT_NEAR(list.circles[0].center.y, center.y, 0.01f);
    EXPECT_NEAR(list.circles[0].radius, 12.0f * camera.pixelsPerUnit, 0.01f);
    EXPECT_EQ(list.circles[0].fill.r, 70);
    EXPECT_EQ(list.circles[0].fill.g, 140);
    EXPECT_EQ(list.circles[0].fill.b, 230);
    EXPECT_EQ(list.circles[1].fill.r, 220);
    EXPECT_EQ(list.circles[1].fill.b, 80);
}

TEST(SnapshotRendererTest, RasterizedTeamOneCenterIsBlueAndFarPixelIsMapFill)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    DrawList list = BuildSnapshotDrawList(*scene.state, camera);
    Framebuffer fb;
    fb.width = 1280;
    fb.height = 720;
    Rasterize(list, fb);
    auto px = [&](int x, int y) {
        const std::uint8_t* p = &fb.rgba[(y * fb.width + x) * 4];
        return Rgba{p[0], p[1], p[2], p[3]};
    };
    const int cx = 427;
    const int cy = 360;
    EXPECT_EQ(px(cx, cy).r, 70);
    EXPECT_EQ(px(cx, cy).g, 140);
    EXPECT_EQ(px(cx, cy).b, 230);
    EXPECT_EQ(px(cx + 30, cy).r, 32);
    EXPECT_EQ(px(cx + 30, cy).g, 36);
    EXPECT_EQ(px(cx + 30, cy).b, 42);
}

TEST(SnapshotRendererTest, SelectedUnitGetsYellowOutlineRingLargerThanFill)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    scene.state->GetAllUnits()[0]->SetSelected(true);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    DrawList list = BuildSnapshotDrawList(*scene.state, camera);
    ASSERT_GE(list.circles.size(), 1u);
    const DrawCircle& circle = list.circles[0];
    EXPECT_EQ(circle.outline.r, 255);
    EXPECT_EQ(circle.outline.g, 220);
    EXPECT_EQ(circle.outline.b, 40);
    EXPECT_NE(circle.outline.r, circle.fill.r);

    Framebuffer fb;
    fb.width = 1280;
    fb.height = 720;
    Rasterize(list, fb);
    auto px = [&](int x, int y) {
        const std::uint8_t* p = &fb.rgba[(y * fb.width + x) * 4];
        return Rgba{p[0], p[1], p[2], p[3]};
    };
    const int cx = 427;
    const int cy = 360;
    const int ringSampleX = cx + static_cast<int>(std::floor(circle.radius)) + 2;
    EXPECT_EQ(px(cx, cy).r, circle.fill.r);
    EXPECT_EQ(px(ringSampleX, cy).r, 255);
    EXPECT_EQ(px(ringSampleX, cy).g, 220);
    EXPECT_EQ(px(ringSampleX, cy).b, 40);
}
