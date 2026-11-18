#include "UiCanvas.h"

#include "Font5x7Printable.h"

#include <algorithm>
#include <cmath>

namespace
{

int RowStridePixels(const Framebuffer& fb)
{
    return fb.width + fb.guardColumnsRight;
}

void PutPixel(Framebuffer& fb, int x, int y, Rgba8 color)
{
    if (x < 0 || y < 0 || x >= fb.width || y >= fb.height)
    {
        return;
    }
    const size_t i = static_cast<size_t>((y * RowStridePixels(fb) + x) * 4);
    if (i + 3 >= fb.rgba.size())
    {
        return;
    }
    fb.rgba[i] = color.r;
    fb.rgba[i + 1] = color.g;
    fb.rgba[i + 2] = color.b;
    fb.rgba[i + 3] = color.a;
}

void FillRect(Framebuffer& fb, int x, int y, int w, int h, Rgba8 color)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    const int x1 = x + w;
    const int y1 = y + h;
    for (int py = y; py < y1; ++py)
    {
        for (int px = x; px < x1; ++px)
        {
            PutPixel(fb, px, py, color);
        }
    }
}

} // namespace

void DrawRectFilled(Framebuffer& fb, int x, int y, int w, int h, Rgba8 color)
{
    FillRect(fb, x, y, w, h, color);
}

void DrawRectOutline(Framebuffer& fb, int x, int y, int w, int h, int thickness, Rgba8 color)
{
    if (w <= 0 || h <= 0 || thickness <= 0)
    {
        return;
    }
    const int t = std::min({thickness, w, h});
    DrawRectFilled(fb, x, y, w, t, color);
    DrawRectFilled(fb, x, y + h - t, w, t, color);
    DrawRectFilled(fb, x, y, t, h, color);
    DrawRectFilled(fb, x + w - t, y, t, h, color);
}

void DrawLine(Framebuffer& fb, int x0, int y0, int x1, int y1, Rgba8 color)
{
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int x = x0;
    int y = y0;

    while (true)
    {
        PutPixel(fb, x, y, color);
        if (x == x1 && y == y1)
        {
            break;
        }
        const int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }
}

void DrawProgressBar(
    Framebuffer& fb,
    int x,
    int y,
    int w,
    int h,
    float fraction,
    Rgba8 fgColor,
    Rgba8 bgColor)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    DrawRectFilled(fb, x, y, w, h, bgColor);
    const int fillW = static_cast<int>(std::lround(static_cast<float>(w) * fraction));
    if (fillW > 0)
    {
        DrawRectFilled(fb, x, y, fillW, h, fgColor);
    }
}

void DrawText5x7(Framebuffer& fb, int x, int y, std::string_view text, int scale, Rgba8 color)
{
    if (scale <= 0)
    {
        return;
    }
    int cursorX = x;
    constexpr int kGlyphWidth = 5;
    constexpr int kGlyphAdvance = 6;
    for (unsigned char raw : text)
    {
        const std::uint8_t* glyph = Font5x7PrintableGlyph(raw);
        for (int row = 0; row < 7; ++row)
        {
            const std::uint8_t bits = glyph[row];
            for (int col = 0; col < kGlyphWidth; ++col)
            {
                if ((bits >> (4 - col)) & 1)
                {
                    const int px = cursorX + col * scale;
                    const int py = y + row * scale;
                    FillRect(fb, px, py, scale, scale, color);
                }
            }
        }
        cursorX += kGlyphAdvance * scale;
    }
}

bool HitTestRect(int mouseX, int mouseY, int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0)
    {
        return false;
    }
    return mouseX >= x && mouseX < x + w && mouseY >= y && mouseY < y + h;
}