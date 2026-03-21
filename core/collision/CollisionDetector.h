#pragma once

#include "ICollisionDetector.h"
#include "SpatialGrid.h"
#include "NarrowPhaseDetector.h"
#include "../interfaces/IGameUnit.h"
#include <vector>

class CollisionDetector : public ICollisionDetector {
private:
    float cellSize;
    float maxAgentRadius;

public:
    CollisionDetector(float cellSize = 50.0f);

    std::vector<CollisionContact> DetectCollisions(
        const std::vector<IGameUnit*>& units,
        const MapBounds& bounds
    ) override;

    void SetCellSize(float size) { cellSize = size; }
    float GetCellSize() const { return cellSize; }

private:
    float CalculateMaxRadius(const std::vector<IGameUnit*>& units) const;
    float CalculateOptimalCellSize(const std::vector<IGameUnit*>& units) const;
};

