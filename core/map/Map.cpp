#include "Map.h"
#include <algorithm>
#include <cmath>

Map::Map(Vector2 size, MapShape mapShape, const std::string& id, const std::string& name)
    : dimensions(size), shape(mapShape), mapId(id), mapName(name) {
}

void Map::AddEntity(std::unique_ptr<MapEntity> entity) {
    if (entity) {
        entities.push_back(std::move(entity));
    }
}

void Map::RemoveEntity(const std::string& entityId) {
    auto it = std::remove_if(entities.begin(), entities.end(),
        [&entityId](const std::unique_ptr<MapEntity>& entity) {
            return entity->GetId() == entityId;
        });
    entities.erase(it, entities.end());
}

MapEntity* Map::GetEntity(const std::string& entityId) {
    for (auto& entity : entities) {
        if (entity->GetId() == entityId) {
            return entity.get();
        }
    }
    return nullptr;
}

const MapEntity* Map::GetEntity(const std::string& entityId) const {
    for (const auto& entity : entities) {
        if (entity->GetId() == entityId) {
            return entity.get();
        }
    }
    return nullptr;
}

std::vector<MapEntity*> Map::GetEntitiesInRadius(Vector2 position, float radius) const {
    std::vector<MapEntity*> result;
    for (const auto& entity : entities) {
        float distance = entity->GetPosition().Distance(position);
        if (distance <= radius) {
            result.push_back(entity.get());
        }
    }
    return result;
}

bool Map::IsValidPosition(Vector2 position) const {
    switch (shape) {
        case MapShape::RECTANGLE:
            return position.x >= 0.0f && position.x <= dimensions.x &&
                   position.y >= 0.0f && position.y <= dimensions.y;

        case MapShape::CIRCLE: {
            Vector2 center = GetCenter();
            float radius = std::min(dimensions.x, dimensions.y) * 0.5f;
            return position.Distance(center) <= radius;
        }

        case MapShape::CUSTOM_POLYGON:
            // TODO: Implement polygon check when custom shapes are supported
            return position.x >= 0.0f && position.x <= dimensions.x &&
                   position.y >= 0.0f && position.y <= dimensions.y;

        default:
            return false;
    }
}

