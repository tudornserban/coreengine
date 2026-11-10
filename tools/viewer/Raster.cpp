#include "Raster.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace
{

int RowStridePixels(const Framebuffer& fb)
{
    return fb.width + fb.guardColumnsRight;
}

int StorageRows(const Framebuffer& fb)
{
    return fb.height + fb.guardRowsBelow;
}

size_t PixelByteIndex(const Framebuffer& fb, int x, int y)
{
    return static_cast<size_t>((y * RowStridePixels(fb) + x) * 4);
}

void WriteGuardRegions(Framebuffer& fb)
{
    if (fb.guardRowsBelow > 0)
    {
        for (int x = 0; x < fb.width; ++x)
        {
            const size_t i = PixelByteIndex(fb, x, fb.height);
            fb.rgba[i] = kRasterGuardSentinel.r;
            fb.rgba[i + 1] = kRasterGuardSentinel.g;
            fb.rgba[i + 2] = kRasterGuardSentinel.b;
            fb.rgba[i + 3] = kRasterGuardSentinel.a;
        }
    }
    if (fb.guardColumnsRight > 0)
    {
        const int guardX = fb.width;
        for (int y = 0; y < StorageRows(fb); ++y)
        {
            const size_t i = PixelByteIndex(fb, guardX, y);
            fb.rgba[i] = kRasterGuardSentinel.r;
            fb.rgba[i + 1] = kRasterGuardSentinel.g;
            fb.rgba[i + 2] = kRasterGuardSentinel.b;
            fb.rgba[i + 3] = kRasterGuardSentinel.a;
        }
    }
}

void PutPixel(Framebuffer& fb, int x, int y, Rgba color)
{
    if (x < 0 || y < 0 || x >= fb.width || y >= fb.height)
    {
        return;
    }
    const size_t i = PixelByteIndex(fb, x, y);
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

bool RgbaEqual(Rgba a, Rgba b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

void RasterizeCircle(const DrawCircle& circle, Framebuffer& fb)
{
    const int cx = static_cast<int>(std::lround(circle.center.x));
    const int cy = static_cast<int>(std::lround(circle.center.y));
    const float fillRadius = circle.radius;
    const bool selectionRing = !RgbaEqual(circle.fill, circle.outline);
    const float outlineRadius = selectionRing ? fillRadius + 2.0f : fillRadius;
    const int bound = static_cast<int>(std::ceil(outlineRadius));

    for (int y = cy - bound; y <= cy + bound; ++y)
    {
        for (int x = cx - bound; x <= cx + bound; ++x)
        {
            const float dx = static_cast<float>(x - cx);
            const float dy = static_cast<float>(y - cy);
            const float distSq = dx * dx + dy * dy;
            if (distSq <= fillRadius * fillRadius)
            {
                PutPixel(fb, x, y, circle.fill);
            }
            else if (selectionRing && distSq <= outlineRadius * outlineRadius)
            {
                PutPixel(fb, x, y, circle.outline);
            }
        }
    }
    PutPixel(fb, cx, cy, circle.fill);
}

} // namespace

void Rasterize(const DrawList& list, Framebuffer& fb)
{
    if (fb.width <= 0 || fb.height <= 0)
    {
        return;
    }
    const size_t storageBytes =
        static_cast<size_t>(StorageRows(fb) * RowStridePixels(fb) * 4);
    const bool hasGuards = fb.guardRowsBelow > 0 || fb.guardColumnsRight > 0;
    if (hasGuards)
    {
        if (fb.rgba.size() != storageBytes)
        {
            fb.rgba.assign(storageBytes, 0);
            WriteGuardRegions(fb);
        }
    }
    else
    {
        fb.rgba.assign(static_cast<size_t>(fb.width * fb.height * 4), 0);
    }
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
        RasterizeCircle(circle, fb);
    }
}

void FramebufferInitWithBottomRowGuard(Framebuffer& fb, int width, int height)
{
    fb.width = width;
    fb.height = height;
    fb.guardRowsBelow = 1;
    fb.guardColumnsRight = 0;
    const size_t storageBytes =
        static_cast<size_t>(StorageRows(fb) * RowStridePixels(fb) * 4);
    fb.rgba.assign(storageBytes, 0);
    WriteGuardRegions(fb);
}

void FramebufferInitWithRightColumnGuard(Framebuffer& fb, int width, int height)
{
    fb.width = width;
    fb.height = height;
    fb.guardRowsBelow = 0;
    fb.guardColumnsRight = 1;
    const size_t storageBytes =
        static_cast<size_t>(StorageRows(fb) * RowStridePixels(fb) * 4);
    fb.rgba.assign(storageBytes, 0);
    WriteGuardRegions(fb);
}

bool FramebufferGuardRowBelowIsClean(const Framebuffer& fb)
{
    if (fb.guardRowsBelow <= 0)
    {
        return true;
    }
    for (int x = 0; x < fb.width; ++x)
    {
        const size_t i = PixelByteIndex(fb, x, fb.height);
        if (fb.rgba[i] != kRasterGuardSentinel.r || fb.rgba[i + 1] != kRasterGuardSentinel.g
            || fb.rgba[i + 2] != kRasterGuardSentinel.b || fb.rgba[i + 3] != kRasterGuardSentinel.a)
        {
            return false;
        }
    }
    return true;
}

bool FramebufferGuardColumnRightIsClean(const Framebuffer& fb)
{
    if (fb.guardColumnsRight <= 0)
    {
        return true;
    }
    const int guardX = fb.width;
    for (int y = 0; y < StorageRows(fb); ++y)
    {
        const size_t i = PixelByteIndex(fb, guardX, y);
        if (fb.rgba[i] != kRasterGuardSentinel.r || fb.rgba[i + 1] != kRasterGuardSentinel.g
            || fb.rgba[i + 2] != kRasterGuardSentinel.b || fb.rgba[i + 3] != kRasterGuardSentinel.a)
        {
            return false;
        }
    }
    return true;
}
