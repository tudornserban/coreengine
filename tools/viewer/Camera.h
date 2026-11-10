#pragma once

#include "core/math/Vector2.h"

struct Camera
{
    Vector2 center;
    float pixelsPerUnit;
    int viewportW;
    int viewportH;
    Vector2 WorldToScreen(Vector2 world) const;
    Vector2 ScreenToWorld(Vector2 screen) const;
    void Pan(Vector2 screenDelta);
    void ZoomAt(Vector2 screen, float factor);
};

Camera FitMap(Vector2 mapSize, int viewportW, int viewportH, float marginPx);
