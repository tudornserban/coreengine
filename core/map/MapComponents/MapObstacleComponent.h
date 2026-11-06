#pragma once

#include "IMapComponent.h"
#include "../../math/Vector2.h"
#include "core/influence_types/InfluenceFieldItem.h"

class MapObstacleComponent : public IMapComponent {
private:
    float radius;
    InfluenceFieldItem influenceFieldItem;

public:
    MapObstacleComponent(float obstacleRadius, const InfluenceFieldItem& fieldItem);

    const char* GetTypeName() const override { return "MapObstacleComponent"; }

    float GetRadius() const { return radius; }
    const InfluenceFieldItem& GetInfluenceFieldItem() const { return influenceFieldItem; }
    void SetInfluenceFieldItem(const InfluenceFieldItem& item) { influenceFieldItem = item; }
};
