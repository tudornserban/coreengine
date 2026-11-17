#pragma once

#include "Vector2.h"

#include <algorithm>
#include <cmath>

/// Arena distance helpers shared by placement, influence, and combat.
/// `Delta` is the vector from `(ax, ay)` to `(bx, by)`. When `wrapBorders` is
/// true, each axis uses the shortest toroidal step across an arena of the
/// given width and height. Non-positive extents disable wrapping on that axis.
struct ArenaGeometry {
    static float WrapAxis(float value, float extent) {
        if (!(extent > 0.0f)) {
            return value;
        }
        float wrapped = std::fmod(value, extent);
        if (wrapped < 0.0f) {
            wrapped += extent;
        }
        return wrapped;
    }

    static Vector2 Delta(float ax, float ay, float bx, float by,
                         float arenaW, float arenaH, bool wrapBorders) {
        float dx = bx - ax;
        float dy = by - ay;
        if (wrapBorders) {
            if (arenaW > 0.0f) {
                if (dx > arenaW * 0.5f) {
                    dx -= arenaW;
                } else if (dx < -arenaW * 0.5f) {
                    dx += arenaW;
                }
            }
            if (arenaH > 0.0f) {
                if (dy > arenaH * 0.5f) {
                    dy -= arenaH;
                } else if (dy < -arenaH * 0.5f) {
                    dy += arenaH;
                }
            }
        }
        return Vector2(dx, dy);
    }

    static float DistanceSq(float ax, float ay, float bx, float by,
                            float arenaW, float arenaH, bool wrapBorders) {
        return Delta(ax, ay, bx, by, arenaW, arenaH, wrapBorders).LengthSquared();
    }

    static float Distance(float ax, float ay, float bx, float by,
                          float arenaW, float arenaH, bool wrapBorders) {
        return std::sqrt(DistanceSq(ax, ay, bx, by, arenaW, arenaH, wrapBorders));
    }

    static Vector2 WrapPosition(float x, float y, float arenaW, float arenaH) {
        return Vector2(WrapAxis(x, arenaW), WrapAxis(y, arenaH));
    }

    static Vector2 ClampPosition(float x, float y, float arenaW, float arenaH) {
        const float maxX = std::max(0.0f, arenaW);
        const float maxY = std::max(0.0f, arenaH);
        return Vector2(
            std::max(0.0f, std::min(maxX, x)),
            std::max(0.0f, std::min(maxY, y)));
    }
};
