#include "MapCollisionHandler.h"
#include "NarrowPhaseDetector.h"
#include "CollisionResolver.h"
#include "../interfaces/IUnitKinematics.h"

bool MapCollisionHandler::DetectAndResolveMapEdge(
    IGameUnit* unit,
    Vector2 mapMin,
    Vector2 mapMax,
    CollisionContact& contact,
    float mapRestitution,
    float mass) {

    if (!unit) return false;

    // Detect collision
    if (!NarrowPhaseDetector::DetectMapEdge(unit, mapMin, mapMax, contact)) {
        return false;
    }

    // Resolve collision
    CollisionResolver::ResolveAgentMap(unit, contact, mapRestitution, mass);

    // Push unit back inside bounds
    PushInsideBounds(unit, mapMin, mapMax);

    return true;
}

void MapCollisionHandler::PushInsideBounds(
    IGameUnit* unit,
    Vector2 mapMin,
    Vector2 mapMax) {

    if (!unit) return;

    IUnitKinematics* kinematics = dynamic_cast<IUnitKinematics*>(unit);
    if (!kinematics) return;

    Vector2 pos = unit->GetPosition();
    float radius = unit->GetRadius();

    // Clamp position to keep unit inside bounds
    pos.x = std::max(mapMin.x + radius, std::min(mapMax.x - radius, pos.x));
    pos.y = std::max(mapMin.y + radius, std::min(mapMax.y - radius, pos.y));

    kinematics->SetPosition(pos);
}

