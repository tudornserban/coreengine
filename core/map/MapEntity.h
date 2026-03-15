#pragma once

#include <vector>
#include <memory>
#include <string>
#include "../math/Vector2.h"
#include "MapComponents/IMapComponent.h"

class MapEntity {
private:
    std::string id;
    Vector2 position;
    std::vector<std::unique_ptr<IMapComponent>> components;

public:
    MapEntity(const std::string& entityId, Vector2 pos);
    virtual ~MapEntity() = default;

    // Getters
    const std::string& GetId() const { return id; }
    Vector2 GetPosition() const { return position; }

    // Setters
    void SetPosition(Vector2 pos) { position = pos; }

    // Component management
    void AddComponent(std::unique_ptr<IMapComponent> component);

    // Query components by type
    template<typename T>
    T* GetComponent() {
        for (auto& comp : components) {
            T* typed = dynamic_cast<T*>(comp.get());
            if (typed) return typed;
        }
        return nullptr;
    }

    template<typename T>
    const T* GetComponent() const {
        for (const auto& comp : components) {
            const T* typed = dynamic_cast<const T*>(comp.get());
            if (typed) return typed;
        }
        return nullptr;
    }

    const std::vector<std::unique_ptr<IMapComponent>>& GetComponents() const { return components; }
};

