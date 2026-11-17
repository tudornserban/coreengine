#pragma once

#include <cstdint>

/// Stateless disc obstacle shared by map entities and placement / territory code.
/// `ContainsPoint` uses straight-line distance. Callers that wrap the arena
/// should compare with `ArenaGeometry` instead of this helper.
enum class ObstacleKind : uint8_t {
    Generic = 0,
    Wall,
    Hazard
};

struct Obstacle {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float radius = 0.0f;
    bool blocksPlacement = true;
    bool blocksInfluence = true;
    ObstacleKind kind = ObstacleKind::Generic;

    [[nodiscard]] bool ContainsPoint(float px, float py) const {
        const float dx = px - x;
        const float dy = py - y;
        return (dx * dx + dy * dy) <= radius * radius;
    }
};
