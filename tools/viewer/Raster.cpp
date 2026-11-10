#include "Raster.h"

#include <cmath>
#include <cstdint>

namespace
{

void PutPixel(Framebuffer& fb, int x, int y, Rgba color)
{
    if (x < 0 || y < 0 || x >= fb.width || y >= fb.height)
    {
        return;
    }
    const size_t i = static_cast<size_t>((y * fb.width + x) * 4);
    fb.rgba[i] = color.r;
    fb.rgba[i + 1] = color.g;
    fb.rgba[i + 2] = color.b;
    fb.rgba[i + 3] = color.a;
}

void DrawHLine(Framebuffer& fb, int x0, int x1, int y, Rgba color)
{
    if (x0 > x1)
    {
        std::swap(x0, x1);
    }
    for (int x = x0; x <= x1; ++x)
    {
        PutPixel(fb, x, y, color);
    }
}

void DrawVLine(Framebuffer& fb, int x, int y0, int y1, Rgba color)
{
    if (y0 > y1)
    {
        std::swap(y0, y1);
    }
    for (int y = y0; y <= y1; ++y)
    {
        PutPixel(fb, x, y, color);
    }
}

void RasterizeRect(const DrawRect& rect, Framebuffer& fb)
{
    const int x0 = static_cast<int>(std::lround(rect.min.x));
    const int y0 = static_cast<int>(std::lround(rect.min.y));
    const int x1 = static_cast<int>(std::lround(rect.max.x));
    const int y1 = static_cast<int>(std::lround(rect.max.y));

    if (rect.filled)
    {
        for (int y = y0; y <= y1; ++y)
        {
            DrawHLine(fb, x0, x1, y, rect.color);
        }
    }
    else
    {
        DrawHLine(fb, x0, x1, y0, rect.color);
        DrawHLine(fb, x0, x1, y1, rect.color);
        DrawVLine(fb, x0, y0, y1, rect.color);
        DrawVLine(fb, x1, y0, y1, rect.color);
    }
}

void RasterizeLine(const DrawLine& line, Framebuffer& fb)
{
    const int x0 = static_cast<int>(std::lround(line.a.x));
    const int y0 = static_cast<int>(std::lround(line.a.y));
    const int x1 = static_cast<int>(std::lround(line.b.x));
    const int y1 = static_cast<int>(std::lround(line.b.y));

    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int x = x0;
    int y = y0;

    while (true)
    {
        PutPixel(fb, x, y, line.color);
        if (x == x1 && y == y1)
        {
            break;
        }
        int e2 = 2 * err;
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

} // namespace

void Rasterize(const DrawList& list, Framebuffer& fb)
{
    if (fb.width <= 0 || fb.height <= 0)
    {
        return;
    }
    fb.rgba.assign(static_cast<size_t>(fb.width * fb.height * 4), 0);
    for (int y = 0; y < fb.height; ++y)
    {
        for (int x = 0; x < fb.width; ++x)
        {
            PutPixel(fb, x, y, list.clear);
        }
    }

    for (const DrawRect& rect : list.rects)
    {
        RasterizeRect(rect, fb);
    }
    for (const DrawLine& line : list.lines)
    {
        RasterizeLine(line, fb);
    }
    for (const DrawCircle& circle : list.circles)
    {
        const int cx = static_cast<int>(std::lround(circle.center.x));
        const int cy = static_cast<int>(std::lround(circle.center.y));
        const int r = static_cast<int>(std::lround(circle.radius));
        for (int y = cy - r; y <= cy + r; ++y)
        {
            for (int x = cx - r; x <= cx + r; ++x)
            {
                float dx = static_cast<float>(x - cx);
                float dy = static_cast<float>(y - cy);
                if (dx * dx + dy * dy <= static_cast<float>(r * r))
                {
                    PutPixel(fb, x, y, circle.fill);
                }
            }
        }
        PutPixel(fb, cx, cy, circle.fill);
    }
}
