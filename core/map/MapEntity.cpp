#include "MapEntity.h"

MapEntity::MapEntity(const std::string& entityId, Vector2 pos)
    : id(entityId), position(pos) {
}

void MapEntity::AddComponent(std::unique_ptr<IMapComponent> component) {
    if (component) {
        components.push_back(std::move(component));
    }
}

