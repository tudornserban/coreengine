#include "Explosion.h"
#include <algorithm>
#include <cmath>

Vector2 Explosion::CalculateImpulse(Vector2 unitPosition, float unitMass) const 
{
    // If strength is zero, no impulse
    if (strength_ == 0.0f) 
    {
        return Vector2::Zero();
    }

    // Calculate distance from explosion center to unit
    Vector2 direction = unitPosition - position_;
    float distance = direction.Length();

    // If unit is at explosion center or outside radius, no impulse
    if (distance <= 0.0f || distance > radius_) 
    {
        return Vector2::Zero();
    }

    // Normalize direction
    direction = direction / distance;

    // Calculate falloff factor based on distance
    // Normalized distance (0.0 at center, 1.0 at edge)
    float normalizedDistance = distance / radius_;
    
    // Falloff: (1 - normalizedDistance)^falloffExponent
    // At center (distance=0): falloff = 1.0 (full strength)
    // At edge (distance=radius): falloff = 0.0 (no strength)
    float falloffFactor = std::pow(1.0f - normalizedDistance, falloffExponent_);
    
    // Calculate base impulse magnitude
    float impulseMagnitude = strength_ * falloffFactor;
    
    // Scale by inverse mass (heavier units move less)
    // Use a reference mass (100.0) to normalize the effect
    float massFactor = 100.0f / std::max(unitMass, 1.0f);
    impulseMagnitude *= massFactor;

    // Return impulse vector (direction * magnitude)
    return direction * impulseMagnitude;
}

bool Explosion::IsInRadius(Vector2 position) const 
{
    float distance = position_.Distance(position);
    return distance <= radius_;
}

