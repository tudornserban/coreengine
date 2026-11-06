#pragma once

#include "core/math/Vector2.h"
#include <cmath>

class Explosion 
{
public:
    // Default explosion strength constant
    static constexpr float DEFAULT_EXPLOSION_STRENGTH = 0.0f;

private:
    Vector2 position_;
    float radius_;
    float strength_;
    float falloffExponent_;

public:
    Explosion(Vector2 position, float radius, float strength = DEFAULT_EXPLOSION_STRENGTH, float falloffExponent = 1.0f)
        : position_(position)
        , radius_(radius)
        , strength_(strength)
        , falloffExponent_(falloffExponent)
    {
    }

    // Calculate impulse vector for a unit at the given position
    // Impulse is scaled by unit mass (heavier units move less)
    // Direction is from explosion center to unit position
    Vector2 CalculateImpulse(Vector2 unitPosition, float unitMass) const;

    // Check if a position is within the explosion radius
    bool IsInRadius(Vector2 position) const;

    // Getters (for rendering and queries)
    Vector2 GetPosition() const { return position_; }
    float GetRadius() const { return radius_; }
    float GetStrength() const { return strength_; }
    float GetFalloffExponent() const { return falloffExponent_; }
};

