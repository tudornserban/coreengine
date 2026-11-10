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

TEST(CameraTest, PanShiftsWorldCenterOppositeToScreenDrag)
{
    Camera camera = FitMap(Vector2(800.0f, 600.0f), 1280, 720, 40.0f);
    const Vector2 before = camera.center;
    camera.Pan(Vector2(64.0f, 0.0f));
    EXPECT_LT(camera.center.x, before.x);
}

TEST(CameraTest, ZoomAtKeepsWorldPointUnderCursorFixed)
{
    Camera camera = FitMap(Vector2(800.0f, 600.0f), 1280, 720, 40.0f);
    const Vector2 screen(640.0f, 360.0f);
    const Vector2 worldBefore = camera.ScreenToWorld(screen);
    camera.ZoomAt(screen, 1.25f);
    const Vector2 worldAfter = camera.ScreenToWorld(screen);
    EXPECT_NEAR(worldAfter.x, worldBefore.x, 0.02f);
    EXPECT_NEAR(worldAfter.y, worldBefore.y, 0.02f);
    EXPECT_GT(camera.pixelsPerUnit, 640.0f / 600.0f);
}
