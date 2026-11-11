#pragma once

#include <cstdint>
#include <vector>

#include "core/math/Vector2.h"

struct Rgba
{
    std::uint8_t r, g, b, a;
};

struct DrawCircle
{
    Vector2 center;
    float radius;
    Rgba fill;
    Rgba outline;
};

struct DrawRect
{
    Vector2 min;
    Vector2 max;
    bool filled;
    Rgba color;
};

struct DrawLine
{
    Vector2 a;
    Vector2 b;
    Rgba color;
};

struct DrawList
{
    Rgba clear;
    std::vector<DrawRect> rects;
    std::vector<DrawCircle> circles;
    std::vector<DrawLine> lines;
};

struct Framebuffer
{
    int width = 0;
    int height = 0;
    /// Rows stored below the logical height (sentinel guard); tests set this before Rasterize.
    int guardRowsBelow = 0;
    /// Extra pixel columns per row stored to the right of logical width (sentinel guard).
    int guardColumnsRight = 0;
    std::vector<std::uint8_t> rgba;
};

constexpr int kViewportW = 1280;
constexpr int kViewportH = 720;
constexpr float kFixedDt = 1.0f / 30.0f;
constexpr std::uint64_t kDefaultSeed = 1;
constexpr float kFitMarginPx = 40.0f;
