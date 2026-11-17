#pragma once

#include "IMapComponent.h"
#include "../../math/Vector2.h"
#include "core/influence_types/InfluenceFieldItem.h"
#include "core/map/Obstacle.h"

class MapObstacleComponent : public IMapComponent {
private:
    float radius;
    InfluenceFieldItem influenceFieldItem;
    int obstacleId = 0;
    bool blocksPlacement = true;
    bool blocksInfluence = true;
    ObstacleKind kind = ObstacleKind::Generic;

public:
    MapObstacleComponent(float obstacleRadius, const InfluenceFieldItem& fieldItem);

    const char* GetTypeName() const override { return "MapObstacleComponent"; }

    float GetRadius() const { return radius; }
    const InfluenceFieldItem& GetInfluenceFieldItem() const { return influenceFieldItem; }
    void SetInfluenceFieldItem(const InfluenceFieldItem& item) { influenceFieldItem = item; }

    void SetObstacleId(int id) { obstacleId = id; }
    void SetBlocksPlacement(bool blocks) { blocksPlacement = blocks; }
    void SetBlocksInfluence(bool blocks) { blocksInfluence = blocks; }
    void SetKind(ObstacleKind obstacleKind) { kind = obstacleKind; }

    /// World-space disc used by placement and territory code. Position comes
    /// from the owning map entity; the component stores radius and flags.
    [[nodiscard]] Obstacle ToObstacle(Vector2 worldPosition) const {
        Obstacle obstacle;
        obstacle.id = obstacleId;
        obstacle.x = worldPosition.x;
        obstacle.y = worldPosition.y;
        obstacle.radius = radius;
        obstacle.blocksPlacement = blocksPlacement;
        obstacle.blocksInfluence = blocksInfluence;
        obstacle.kind = kind;
        return obstacle;
    }
};