#include "ImmunityManager.h"
#include <algorithm>
#include <vector>

ImmunityManager::ImmunityManager(int defaultFrames)
    : defaultImmunityFrames(defaultFrames) {
}

void ImmunityManager::SetImmunity(IGameUnit* unit, IGameUnit* againstUnit, int frames) {
    if (!unit || !againstUnit || unit == againstUnit) return;

    // Set immunity for unit against againstUnit
    auto& immunityData = immunityMap[unit];
    immunityData.immuneTo.insert(againstUnit);
    immunityData.remainingFrames = std::max(immunityData.remainingFrames, frames);

    // Also set reciprocal immunity (both must be immune to each other)
    auto& otherImmunityData = immunityMap[againstUnit];
    otherImmunityData.immuneTo.insert(unit);
    otherImmunityData.remainingFrames = std::max(otherImmunityData.remainingFrames, frames);
}

bool ImmunityManager::ShouldGenerateEvent(IGameUnit* agentA, IGameUnit* agentB) const {
    // Only check immunity if both are agents (not map collision)
    if (!agentA || !agentB) return true;

    // Check if both are immune to each other
    auto itA = immunityMap.find(agentA);
    auto itB = immunityMap.find(agentB);

    if (itA == immunityMap.end() || itB == immunityMap.end()) {
        return true;  // No immunity data, allow event
    }

    const ImmunityData& immunityA = itA->second;
    const ImmunityData& immunityB = itB->second;

    // Both must be immune to each other AND immunity must still be active
    bool aImmuneToB = immunityA.immuneTo.find(agentB) != immunityA.immuneTo.end() &&
                       immunityA.remainingFrames > 0;
    bool bImmuneToA = immunityB.immuneTo.find(agentA) != immunityB.immuneTo.end() &&
                       immunityB.remainingFrames > 0;

    // Only prevent event if BOTH are immune to each other
    return !(aImmuneToB && bImmuneToA);
}

void ImmunityManager::Update() {
    // Decrement remaining frames for all immunities
    std::vector<IGameUnit*> unitsToClean;

    for (auto& [unit, immunityData] : immunityMap) {
        if (immunityData.remainingFrames > 0) {
            immunityData.remainingFrames--;
        }

        // Clear expired immunities
        if (immunityData.remainingFrames <= 0) {
            immunityData.immuneTo.clear();
            unitsToClean.push_back(unit);
        }
    }

    // Remove units with no active immunities
    for (IGameUnit* unit : unitsToClean) {
        auto it = immunityMap.find(unit);
        if (it != immunityMap.end() && it->second.immuneTo.empty()) {
            immunityMap.erase(it);
        }
    }
}

void ImmunityManager::Clear() {
    immunityMap.clear();
}

void ImmunityManager::ClearImmunity(IGameUnit* unit) {
    if (!unit) return;

    auto it = immunityMap.find(unit);
    if (it != immunityMap.end()) {
        // Clear reciprocal immunities
        for (IGameUnit* otherUnit : it->second.immuneTo) {
            auto otherIt = immunityMap.find(otherUnit);
            if (otherIt != immunityMap.end()) {
                otherIt->second.immuneTo.erase(unit);
            }
        }

        immunityMap.erase(it);
    }
}

