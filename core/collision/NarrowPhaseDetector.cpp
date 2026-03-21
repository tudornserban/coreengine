#include "NarrowPhaseDetector.h"
#include <algorithm>
#include <cmath>

bool NarrowPhaseDetector::DetectCircleCircle(
    IGameUnit* unitA,
    IGameUnit* unitB,
    CollisionContact& contact) {

    if (!unitA || !unitB) return false;

    Vector2 posA = unitA->GetPosition();
    Vector2 posB = unitB->GetPosition();
    float radiusA = unitA->GetRadius();
    float radiusB = unitB->GetRadius();

    Vector2 diff = posA - posB;
    float distanceSq = diff.LengthSquared();
    float combinedRadius = radiusA + radiusB;
    float combinedRadiusSq = combinedRadius * combinedRadius;

    if (distanceSq < combinedRadiusSq) {
        // Collision detected
        contact.unitA = unitA;
        contact.unitB = unitB;
        contact.type = CollisionType::AGENT_AGENT;

        CalculateCircleCircleContact(posA, radiusA, posB, radiusB, contact);
        return true;
    }

    return false;
}

bool NarrowPhaseDetector::DetectCircleAABB(
    IGameUnit* unit,
    Vector2 aabbMin,
    Vector2 aabbMax,
    CollisionContact& contact) {

    if (!unit) return false;

    Vector2 pos = unit->GetPosition();
    float radius = unit->GetRadius();

    // Find closest point on AABB to circle center
    Vector2 closest = ClosestPointOnAABB(pos, aabbMin, aabbMax);

    // Check if circle overlaps AABB
    Vector2 diff = pos - closest;
    float distanceSq = diff.LengthSquared();

    if (distanceSq < radius * radius) {
        // Collision detected
        contact.unitA = unit;
        contact.unitB = nullptr;
        contact.type = CollisionType::AGENT_MAP_EDGE;

        float distance = std::sqrt(distanceSq);
        if (distance > 0.0001f) {
            contact.normal = diff / distance;
        } else {
            // Circle center is inside AABB, push out in direction from center
            Vector2 aabbCenter = (aabbMin + aabbMax) * 0.5f;
            Vector2 toCenter = pos - aabbCenter;
            if (toCenter.LengthSquared() < 0.0001f) {
                toCenter = Vector2(1.0f, 0.0f);  // Default direction
            }
            contact.normal = toCenter.Normalized();
        }

        contact.penetration = radius - distance;
        contact.contactPoint = closest;
        return true;
    }

    return false;
}

bool NarrowPhaseDetector::DetectMapEdge(
    IGameUnit* unit,
    Vector2 mapMin,
    Vector2 mapMax,
    CollisionContact& contact) {

    if (!unit) return false;

    Vector2 pos = unit->GetPosition();
    float radius = unit->GetRadius();

    // Check each edge
    bool collision = false;

    // Left edge
    if (pos.x - radius < mapMin.x) {
        contact.unitA = unit;
        contact.unitB = nullptr;
        contact.type = CollisionType::AGENT_MAP_EDGE;
        contact.normal = Vector2(1.0f, 0.0f);  // Pointing right (from edge to agent)
        contact.penetration = (pos.x - radius) - mapMin.x;
        contact.contactPoint = Vector2(mapMin.x, pos.y);
        collision = true;
    }
    // Right edge
    else if (pos.x + radius > mapMax.x) {
        contact.unitA = unit;
        contact.unitB = nullptr;
        contact.type = CollisionType::AGENT_MAP_EDGE;
        contact.normal = Vector2(-1.0f, 0.0f);  // Pointing left (from edge to agent)
        contact.penetration = (pos.x + radius) - mapMax.x;
        contact.contactPoint = Vector2(mapMax.x, pos.y);
        collision = true;
    }
    // Top edge
    if (pos.y - radius < mapMin.y) {
        contact.unitA = unit;
        contact.unitB = nullptr;
        contact.type = CollisionType::AGENT_MAP_EDGE;
        // If already colliding horizontally, use combined normal
        if (collision) {
            contact.normal += Vector2(0.0f, 1.0f);
            contact.normal.Normalize();
        } else {
            contact.normal = Vector2(0.0f, 1.0f);
        }
        contact.penetration = std::max(contact.penetration, (pos.y - radius) - mapMin.y);
        contact.contactPoint = Vector2(pos.x, mapMin.y);
        collision = true;
    }
    // Bottom edge
    else if (pos.y + radius > mapMax.y) {
        contact.unitA = unit;
        contact.unitB = nullptr;
        contact.type = CollisionType::AGENT_MAP_EDGE;
        // If already colliding horizontally, use combined normal
        if (collision) {
            contact.normal += Vector2(0.0f, -1.0f);
            contact.normal.Normalize();
        } else {
            contact.normal = Vector2(0.0f, -1.0f);
        }
        contact.penetration = std::max(contact.penetration, (pos.y + radius) - mapMax.y);
        contact.contactPoint = Vector2(pos.x, mapMax.y);
        collision = true;
    }

    return collision;
}

void NarrowPhaseDetector::CalculateCircleCircleContact(
    Vector2 posA, float radiusA,
    Vector2 posB, float radiusB,
    CollisionContact& contact) {

    Vector2 diff = posA - posB;
    float distance = diff.Length();

    if (distance < 0.0001f) {
        // Circles are exactly overlapping, use default direction
        contact.normal = Vector2(1.0f, 0.0f);
        contact.contactPoint = posA;
        contact.penetration = radiusA + radiusB;
    } else {
        // Normal pointing from B to A
        contact.normal = diff / distance;

        // Contact point is on the line between centers, weighted by radius
        float t = radiusA / (radiusA + radiusB);
        contact.contactPoint = Vector2::Lerp(posB, posA, t);

        // Penetration depth
        contact.penetration = (radiusA + radiusB) - distance;
    }
}

Vector2 NarrowPhaseDetector::ClosestPointOnAABB(Vector2 point, Vector2 aabbMin, Vector2 aabbMax) {
    Vector2 closest;
    closest.x = std::max(aabbMin.x, std::min(point.x, aabbMax.x));
    closest.y = std::max(aabbMin.y, std::min(point.y, aabbMax.y));
    return closest;
}

