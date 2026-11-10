#include "Camera.h"

#include <gtest/gtest.h>

#include "core/math/Vector2.h"

TEST(CameraTest, FitMapUsesTheTighterAxisAndCentersTheMap)
{
    Camera camera = FitMap(Vector2(800.0f, 600.0f), 1280, 720, 40.0f);
    EXPECT_FLOAT_EQ(camera.pixelsPerUnit, 640.0f / 600.0f);
    EXPECT_FLOAT_EQ(camera.center.x, 400.0f);
    EXPECT_FLOAT_EQ(camera.center.y, 300.0f);
}

TEST(CameraTest, WorldScreenRoundTripAtMapCenterAndCorner)
{
    Camera camera = FitMap(Vector2(800.0f, 600.0f), 1280, 720, 40.0f);
    Vector2 screen = camera.WorldToScreen(Vector2(400.0f, 300.0f));
    EXPECT_FLOAT_EQ(screen.x, 640.0f);
    EXPECT_FLOAT_EQ(screen.y, 360.0f);
    Vector2 world = camera.ScreenToWorld(camera.WorldToScreen(Vector2(0.0f, 0.0f)));
    EXPECT_NEAR(world.x, 0.0f, 0.001f);
    EXPECT_NEAR(world.y, 0.0f, 0.001f);
}

TEST(CameraTest, PanAppliesLiteralWorldShiftForScreenDelta)
{
    Camera camera = FitMap(Vector2(800.0f, 600.0f), 1280, 720, 40.0f);
    EXPECT_FLOAT_EQ(camera.center.x, 400.0f);
    EXPECT_FLOAT_EQ(camera.center.y, 300.0f);
    camera.Pan(Vector2(64.0f, -32.0f));
    EXPECT_FLOAT_EQ(camera.center.x, 340.0f);
    EXPECT_FLOAT_EQ(camera.center.y, 330.0f);
}

TEST(CameraTest, ZoomAtOffCenterScreenKeepsWorldAnchorFixed)
{
    Camera camera = FitMap(Vector2(800.0f, 600.0f), 1280, 720, 40.0f);
    const float basePixelsPerUnit = 640.0f / 600.0f;
    EXPECT_FLOAT_EQ(camera.pixelsPerUnit, basePixelsPerUnit);
    const Vector2 screen(100.0f, 80.0f);
    const Vector2 worldBefore = camera.ScreenToWorld(screen);
    constexpr float kZoomFactor = 1.25f;
    camera.ZoomAt(screen, kZoomFactor);
    const Vector2 worldAfter = camera.ScreenToWorld(screen);
    EXPECT_NEAR(worldAfter.x, worldBefore.x, 0.02f);
    EXPECT_NEAR(worldAfter.y, worldBefore.y, 0.02f);
    EXPECT_NEAR(camera.pixelsPerUnit, basePixelsPerUnit * kZoomFactor, 0.0001f);
}
