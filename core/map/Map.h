#pragma once

#include <vector>
#include <memory>
#include <string>
#include "../math/Vector2.h"
#include "MapEntity.h"

enum class MapShape {
    RECTANGLE,
    CIRCLE,
    CUSTOM_POLYGON
};

class Map {
private:
    Vector2 dimensions;
    MapShape shape;
    std::vector<std::unique_ptr<MapEntity>> entities;
    std::string mapId;
    std::string mapName;

public:
    Map(Vector2 size, MapShape mapShape = MapShape::RECTANGLE,
        const std::string& id = "", const std::string& name = "");

    // Getters
    Vector2 GetDimensions() const { return dimensions; }
    Vector2 GetCenter() const { return dimensions * 0.5f; }
    MapShape GetShape() const { return shape; }
    const std::string& GetMapId() const { return mapId; }
    const std::string& GetMapName() const { return mapName; }

    // Setters
    void SetDimensions(Vector2 size) { dimensions = size; }
    void SetShape(MapShape newShape) { shape = newShape; }
    void SetMapName(const std::string& name) { mapName = name; }

    // Entity management
    void AddEntity(std::unique_ptr<MapEntity> entity);
    void RemoveEntity(const std::string& entityId);
    MapEntity* GetEntity(const std::string& entityId);
    const MapEntity* GetEntity(const std::string& entityId) const;

    const std::vector<std::unique_ptr<MapEntity>>& GetEntities() const { return entities; }
    std::vector<MapEntity*> GetEntitiesInRadius(Vector2 position, float radius) const;

    // Utility
    bool IsValidPosition(Vector2 position) const;

    // Load/Save (structure ready, implementation deferred)
    // bool LoadFromFile(const std::string& filename);
    // bool SaveToFile(const std::string& filename) const;
};

