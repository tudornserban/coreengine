#include "DrawList.h"
#include "Raster.h"

#include <gtest/gtest.h>

TEST(RasterTest, ClearAndFilledRectPaintThosePixels)
{
    DrawList list;
    list.clear = {20, 24, 28, 255};
    list.rects.push_back({{1, 1}, {2, 2}, true, {32, 36, 42, 255}});
    Framebuffer fb;
    fb.width = 4;
    fb.height = 4;
    Rasterize(list, fb);
    auto px = [&](int x, int y) {
        const std::uint8_t* p = &fb.rgba[(y * fb.width + x) * 4];
        return Rgba{p[0], p[1], p[2], p[3]};
    };
    EXPECT_EQ(px(0, 0).r, 20);
    EXPECT_EQ(px(0, 0).g, 24);
    EXPECT_EQ(px(0, 0).b, 28);
    EXPECT_EQ(px(1, 1).r, 32);
    EXPECT_EQ(px(1, 1).g, 36);
    EXPECT_EQ(px(1, 1).b, 42);
}

TEST(RasterTest, OutOfBoundsRectsLeaveFramebufferAtClearColor)
{
    Framebuffer fb;
    fb.width = 4;
    fb.height = 4;
    DrawList list;
    list.clear = {10, 11, 12, 255};
    list.rects.push_back({{4, 1}, {4, 1}, true, {99, 0, 0, 255}});
    list.rects.push_back({{1, 4}, {1, 4}, true, {0, 99, 0, 255}});
    Rasterize(list, fb);
    auto px = [&](int x, int y) {
        const std::uint8_t* p = &fb.rgba[(y * fb.width + x) * 4];
        return Rgba{p[0], p[1], p[2], p[3]};
    };
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            EXPECT_EQ(px(x, y).r, 10) << "at (" << x << "," << y << ")";
            EXPECT_EQ(px(x, y).g, 11);
            EXPECT_EQ(px(x, y).b, 12);
        }
    }
}

TEST(RasterTest, InBoundsPixelSurvivesOutOfBoundsProbeOnX)
{
    Framebuffer fb;
    fb.width = 4;
    fb.height = 4;
    DrawList list;
    list.clear = {10, 11, 12, 255};
    list.rects.push_back({{2, 1}, {2, 1}, true, {200, 0, 0, 255}});
    list.rects.push_back({{4, 1}, {4, 1}, true, {0, 200, 0, 255}});
    Rasterize(list, fb);
    auto px = [&](int x, int y) {
        const std::uint8_t* p = &fb.rgba[(y * fb.width + x) * 4];
        return Rgba{p[0], p[1], p[2], p[3]};
    };
    EXPECT_EQ(px(2, 1).r, 200);
    EXPECT_EQ(px(0, 1).r, 10);
    EXPECT_EQ(px(3, 1).r, 10);
}

TEST(RasterTest, InBoundsPixelSurvivesOutOfBoundsProbeOnY)
{
    Framebuffer fb;
    fb.width = 4;
    fb.height = 4;
    DrawList list;
    list.clear = {10, 11, 12, 255};
    list.rects.push_back({{1, 2}, {1, 2}, true, {0, 0, 200, 255}});
    list.rects.push_back({{1, 4}, {1, 4}, true, {0, 200, 0, 255}});
    Rasterize(list, fb);
    auto px = [&](int x, int y) {
        const std::uint8_t* p = &fb.rgba[(y * fb.width + x) * 4];
        return Rgba{p[0], p[1], p[2], p[3]};
    };
    EXPECT_EQ(px(1, 2).b, 200);
    EXPECT_EQ(px(1, 0).b, 12);
    EXPECT_EQ(px(1, 3).b, 12);
}
