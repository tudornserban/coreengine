#pragma once

#include <map>
#include <string>
#include <vector>
#include "../influence/InfluenceFieldItem.h"
#include "../math/Vector2.h"

// Forward declarations
struct BehaviorContext;
class IGameUnit;

/// Steering behavior interface.
///
/// Two interaction modes (both supported, choose per behavior):
///
///   1. GenerateFieldItems() — preferred for influence-field based movement.
///      Returns a list of attractors/repulsors consumed by InfluenceFieldCalculator.
///
///   2. CalculateAcceleration() — direct acceleration vector (legacy / simple behaviors).
///      Used when GenerateFieldItems() returns an empty vector.
///
/// RationalInsanity uses the BehaviorFactory to swap behaviors at runtime (magic dust).
class IBehavior {
public:
    virtual ~IBehavior() = default;

    // --- Primary interface (influence-field based) ---
    virtual std::vector<InfluenceFieldItem> GenerateFieldItems(
        const BehaviorContext& ctx
    ) { return {}; }

    // --- Fallback interface (direct acceleration) ---
    virtual Vector2 CalculateAcceleration(const BehaviorContext& ctx) {
        return Vector2::Zero();
    }

    // --- Lifecycle ---
    virtual void OnEnter(IGameUnit* /*unit*/) {}
    virtual void OnExit(IGameUnit* /*unit*/)  {}

    // --- Metadata ---
    virtual const char* GetName() const = 0;

    // --- Runtime parameter tuning ---
    virtual std::map<std::string, float>& GetParameters() = 0;
    virtual const std::map<std::string, float>& GetParameters() const = 0;
};
