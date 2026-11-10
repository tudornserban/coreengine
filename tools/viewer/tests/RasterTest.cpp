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
