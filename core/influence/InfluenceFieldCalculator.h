#pragma once

#include "../math/Vector2.h"
#include "InfluenceFieldItem.h"
#include <vector>

/// Calculates the total acceleration force on an agent from a list of
/// InfluenceFieldItems (attractors, repulsors, edges).
///
/// This is the engine's core movement math — a Potential Field / APF solver.
/// Ported directly from VenusWars PotentialFieldCalculations.
namespace InfluenceField {

    /// Total force from all field items. Result is clamped to maxAcceleration.
    Vector2 CalculateTotalForce(
        const Vector2& agentPos,
        const Vector2& agentVel,
        const std::vector<InfluenceFieldItem>& items,
        float maxAcceleration
    );

    /// Force from a single attractor item.
    Vector2 CalculateAttractorForce(
        const Vector2& agentPos,
        const Vector2& agentVel,
        const InfluenceFieldItem& attractor
    );

    /// Force from a single repulsor item.
    Vector2 CalculateRepulsorForce(
        const Vector2& agentPos,
        const Vector2& agentVel,
        const InfluenceFieldItem& repulsor
    );

    /// Optimized force for map edges and static obstacles.
    Vector2 CalculateEdgeForce(
        const Vector2& agentPos,
        const InfluenceFieldItem& edge
    );

} // namespace InfluenceField
