#pragma once

#include "core/interfaces/IGameUnit.h"
#include <unordered_map>
#include <set>

struct ImmunityData {
    int remainingFrames;
    std::set<IGameUnit*> immuneTo;  // Agents this unit is immune to (for event generation)

    ImmunityData() : remainingFrames(0) {}
};

class ImmunityManager {
private:
    std::unordered_map<IGameUnit*, ImmunityData> immunityMap;
    int defaultImmunityFrames;

public:
    ImmunityManager(int defaultFrames = 3);

    // Set immunity for a unit against another unit
    void SetImmunity(IGameUnit* unit, IGameUnit* againstUnit, int frames);

    // Check if event should be generated (both must be immune to each other)
    bool ShouldGenerateEvent(IGameUnit* agentA, IGameUnit* agentB) const;

    // Update immunity (decrement frames) - call each game update frame
    void Update();

    // Clear all immunity
    void Clear();

    // Clear immunity for a specific unit
    void ClearImmunity(IGameUnit* unit);

    void SetDefaultImmunityFrames(int frames) { defaultImmunityFrames = frames; }
    int GetDefaultImmunityFrames() const { return defaultImmunityFrames; }
};

