#include "DrawList.h"
#include "UiCanvas.h"

#include <gtest/gtest.h>

namespace
{

Rgba ReadPx(const Framebuffer& fb, int x, int y)
{
    const size_t i = static_cast<size_t>((y * fb.width + x) * 4);
    const std::uint8_t* p = &fb.rgba[i];
    return Rgba{p[0], p[1], p[2], p[3]};
}

void InitFramebuffer(Framebuffer& fb, int width, int height)
{
    fb.width = width;
    fb.height = height;
    fb.guardRowsBelow = 0;
    fb.guardColumnsRight = 0;
    fb.rgba.assign(static_cast<size_t>(width * height * 4), 0);
}

} // namespace

TEST(UiCanvasTest, DrawRectFilledPaintsInterior)
{
    Framebuffer fb;
    InitFramebuffer(fb, 8, 8);
    DrawRectFilled(fb, 2, 2, 3, 3, {10, 20, 30, 255});
    EXPECT_EQ(ReadPx(fb, 2, 2).r, 10);
    EXPECT_EQ(ReadPx(fb, 4, 4).g, 20);
    EXPECT_EQ(ReadPx(fb, 1, 2).r, 0);
}

TEST(UiCanvasTest, DrawLineBresenhamDiagonal)
{
    Framebuffer fb;
    InitFramebuffer(fb, 8, 8);
    DrawLine(fb, 0, 0, 4, 4, {200, 0, 0, 255});
    EXPECT_EQ(ReadPx(fb, 0, 0).r, 200);
    EXPECT_EQ(ReadPx(fb, 4, 4).r, 200);
    EXPECT_EQ(ReadPx(fb, 3, 1).r, 0);
}

TEST(UiCanvasTest, DrawProgressBarClampsAndFillsFraction)
{
    Framebuffer fb;
    InitFramebuffer(fb, 20, 6);
    DrawProgressBar(fb, 1, 1, 10, 4, 0.5f, {0, 180, 0, 255}, {40, 40, 40, 255});
    EXPECT_EQ(ReadPx(fb, 1, 2).g, 180);
    EXPECT_EQ(ReadPx(fb, 5, 2).g, 180);
    EXPECT_EQ(ReadPx(fb, 7, 2).r, 40);
}

TEST(UiCanvasTest, DrawText5x7ScaleRendersGlyphPixels)
{
    Framebuffer fb;
    InitFramebuffer(fb, 32, 16);
    DrawText5x7(fb, 2, 2, "A", 2, {255, 255, 255, 255});
    EXPECT_EQ(ReadPx(fb, 4, 2).r, 255);
    EXPECT_EQ(ReadPx(fb, 2, 4).r, 255);
    EXPECT_EQ(ReadPx(fb, 20, 2).r, 0);
}

TEST(UiCanvasTest, HitTestRectRespectsBounds)
{
    EXPECT_TRUE(HitTestRect(5, 5, 0, 0, 10, 10));
    EXPECT_FALSE(HitTestRect(10, 5, 0, 0, 10, 10));
    EXPECT_FALSE(HitTestRect(5, 5, 0, 0, 0, 10));
}