#include "Camera.h"

#include <algorithm>

Vector2 Camera::WorldToScreen(Vector2 world) const
{
    Vector2 d = world - center;
    return Vector2(
        viewportW * 0.5f + d.x * pixelsPerUnit,
        viewportH * 0.5f + d.y * pixelsPerUnit);
}

Vector2 Camera::ScreenToWorld(Vector2 screen) const
{
    return Vector2(
        center.x + (screen.x - viewportW * 0.5f) / pixelsPerUnit,
        center.y + (screen.y - viewportH * 0.5f) / pixelsPerUnit);
}

void Camera::Pan(Vector2 screenDelta)
{
    center.x -= screenDelta.x / pixelsPerUnit;
    center.y -= screenDelta.y / pixelsPerUnit;
}

void Camera::ZoomAt(Vector2 screen, float factor)
{
    if (factor <= 0.0f)
    {
        return;
    }
    const Vector2 anchorWorld = ScreenToWorld(screen);
    pixelsPerUnit *= factor;
    const Vector2 anchorAfter = ScreenToWorld(screen);
    center.x += anchorWorld.x - anchorAfter.x;
    center.y += anchorWorld.y - anchorAfter.y;
}

Camera FitMap(Vector2 mapSize, int viewportW, int viewportH, float marginPx)
{
    Camera camera;
    camera.viewportW = viewportW;
    camera.viewportH = viewportH;

    if (mapSize.x <= 0.0f || mapSize.y <= 0.0f || viewportW <= 0 || viewportH <= 0
        || marginPx * 2.0f >= static_cast<float>(viewportW)
        || marginPx * 2.0f >= static_cast<float>(viewportH))
    {
        camera.center = Vector2(0.0f, 0.0f);
        camera.pixelsPerUnit = 1.0f;
        return camera;
    }

    camera.center = mapSize * 0.5f;
    float usableW = viewportW - 2.0f * marginPx;
    float usableH = viewportH - 2.0f * marginPx;
    camera.pixelsPerUnit = std::min(usableW / mapSize.x, usableH / mapSize.y);
    return camera;
}
