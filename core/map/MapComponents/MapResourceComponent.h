#pragma once

#include "IMapComponent.h"
#include "../../math/Vector2.h"
#include <string>

enum class ResourceType {
    CREDITS,
    ENERGY,
    MINERALS,
    CUSTOM
};

class MapResourceComponent : public IMapComponent {
private:
    ResourceType resourceType;
    float amount;
    float maxAmount;
    bool isDepleted;
    std::string customTypeName;  // For CUSTOM type

public:
    MapResourceComponent(ResourceType type, float initialAmount, float maximumAmount = -1.0f);

    const char* GetTypeName() const override { return "MapResourceComponent"; }

    ResourceType GetResourceType() const { return resourceType; }
    float GetAmount() const { return amount; }
    float GetMaxAmount() const { return maxAmount; }
    bool IsDepleted() const { return isDepleted; }
    const std::string& GetCustomTypeName() const { return customTypeName; }

    void SetAmount(float newAmount);
    void SetCustomTypeName(const std::string& name) { customTypeName = name; }
};

