#include "CollisionDetector.h"
#include "../interfaces/IGameUnit.h"
#include <algorithm>
#include <cmath>

CollisionDetector::CollisionDetector(float cellSize)
    : cellSize(cellSize)
    , maxAgentRadius(0.0f) {
}

std::vector<CollisionContact> CollisionDetector::DetectCollisions(
    const std::vector<IGameUnit*>& units,
    const MapBounds& bounds) {

    std::vector<CollisionContact> contacts;

    if (units.empty()) return contacts;

    // Calculate optimal cell size if needed
    maxAgentRadius = CalculateMaxRadius(units);
    if (cellSize < maxAgentRadius * 2.0f) {
        cellSize = maxAgentRadius * 2.0f;
    }

    // Create spatial grid for broad phase
    SpatialGrid grid(cellSize, bounds.min, bounds.max);

    // Insert all units into grid
    for (IGameUnit* unit : units) {
        if (unit) {
            grid.Insert(unit);
        }
    }

    // Broad phase: Get potential collision pairs
    std::vector<std::pair<IGameUnit*, IGameUnit*>> potentialPairs = grid.GetPotentialPairs();

    // Narrow phase: Check each pair
    for (const auto& pair : potentialPairs) {
        CollisionContact contact;
        if (NarrowPhaseDetector::DetectCircleCircle(pair.first, pair.second, contact)) {
            contacts.push_back(contact);
        }
    }

    // Check map edge collisions (simple checks, no spatial grid needed)
    for (IGameUnit* unit : units) {
        if (!unit) continue;

        CollisionContact edgeContact;
        if (NarrowPhaseDetector::DetectMapEdge(unit, bounds.min, bounds.max, edgeContact)) {
            contacts.push_back(edgeContact);
        }
    }

    return contacts;
}

float CollisionDetector::CalculateMaxRadius(const std::vector<IGameUnit*>& units) const {
    float maxRadius = 0.0f;
    for (IGameUnit* unit : units) {
        if (unit) {
            maxRadius = std::max(maxRadius, unit->GetRadius());
        }
    }
    return maxRadius;
}

float CollisionDetector::CalculateOptimalCellSize(const std::vector<IGameUnit*>& units) const {
    float maxRadius = CalculateMaxRadius(units);
    // Cell size should be at least 2x the largest radius to ensure
    // adjacent cells are checked for collisions
    return std::max(cellSize, maxRadius * 2.0f);
}

