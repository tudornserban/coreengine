#include "MapResourceComponent.h"

MapResourceComponent::MapResourceComponent(ResourceType type, float initialAmount, float maximumAmount)
    : resourceType(type), amount(initialAmount), maxAmount(maximumAmount), isDepleted(false) {
    if (maxAmount < 0.0f) {
        maxAmount = initialAmount;  // Default to initial amount if not specified
    }
}

void MapResourceComponent::SetAmount(float newAmount) {
    amount = newAmount;
    if (maxAmount > 0.0f && amount <= 0.0f) {
        isDepleted = true;
    } else if (amount > 0.0f) {
        isDepleted = false;
    }
}

