#pragma once

#include "core/interfaces/IGameUnit.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

enum class CombatMode : uint8_t {
    InfluenceAttrition,
    DirectAttack,
    Hybrid
};

enum class TargetPolicy : uint8_t {
    Nearest,
    LowestHP,
    HighestThreat
};

/// Simultaneous-resolution combat.
/// Damage is accumulated for the whole tick and applied with `IUnitCombat::TakeDamage`
/// only after every pair and shot has been evaluated, so iteration order does not
/// change who is alive during targeting.
class CombatManager {
public:
    using TargetFilterFn = std::function<bool(const IGameUnit* attacker, const IGameUnit* candidate)>;
    using DamageMultiplierFn = std::function<float(const IGameUnit* attacker, const IGameUnit* defender)>;
    using AreAlliesFn = std::function<bool(const IGameUnit* a, const IGameUnit* b)>;
    using SurroundedFn = std::function<bool(const IGameUnit* unit)>;

    struct Params {
        CombatMode mode = CombatMode::InfluenceAttrition;
        float zoneDamagePerSecond = 0.0f;
        /// Added as `1 + multiplier * closeness^2`, where closeness is 1 at the
        /// same point and 0 at the edge of the summed influence radii.
        float proximityDamageMultiplier = 0.0f;
        /// Applied when center distance is inside the sum of body radii.
        float collisionDamageMultiplier = 1.0f;
        float outnumberedBonusPerExtraAttacker = 0.0f;
        float surroundedHpFractionPerSecond = 0.0f;
        TargetPolicy targetPolicy = TargetPolicy::Nearest;
        TargetFilterFn targetFilter;
        DamageMultiplierFn damageMultiplier;
        AreAlliesFn areAllies;
        SurroundedFn isSurrounded;
        float splashRadius = 0.0f;
        /// 0 keeps full splash damage inside the radius. 1 falls off linearly to 0 at the edge.
        float splashFalloff = 0.0f;
        bool wrapBorders = false;
        float arenaW = 0.0f;
        float arenaH = 0.0f;
    };

    /// Surrounded attrition applied immediately. Units without `IUnitCombat` are ignored.
    void ProcessAttrition(IGameUnit* unit, bool isSurrounded, float deltaTime,
                          float surroundedHpFractionPerSecond);

    /// Resolve one tick. Shot cooldown timers persist on this manager.
    /// Units with a non-zero `GetEntityId()` are keyed by that id so timers survive
    /// pointer changes. Units without an id stay keyed by pointer.
    void Resolve(const std::vector<IGameUnit*>& units, float deltaTime, const Params& params);

    struct CooldownRecord {
        uint64_t entityId = 0;
        float remaining = 0.0f;
    };

    /// Id-keyed timers in ascending entity id order. Pointer-keyed timers are not included.
    std::vector<CooldownRecord> ExportCooldowns() const;
    void ImportCooldowns(const std::vector<CooldownRecord>& records);
    void ForgetEntity(uint64_t entityId);

private:
    float& TimerFor(const IGameUnit* unit);

    std::unordered_map<const IGameUnit*, float> attackTimers_;
    std::unordered_map<uint64_t, float> attackTimersById_;
};
