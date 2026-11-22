#pragma once

#include "core/combat/InteractionMatrix.h"
#include "core/combat/ProjectileSimulation.h"
#include "core/combat/TurnClashResolver.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "platform/unreal/UECoordinateBridge.h"
#include "platform/unreal/UEGenericInventory.h"

#include <cstdint>
#include <string>

namespace CoreEngine {
namespace Unreal {

/// 3D animation / locomotion state derived from the bound CoreEngine RTSUnit.
enum class UECharacterAnimState : uint8_t {
    Idle = 0,
    Moving = 1,
    Attacking = 2,
    CastingTimedAction = 3,
    HitImpulse = 4,
    Dead = 5
};

struct UECharacterBaseStats {
    float maxHP = 100.0f;
    float maxSpeed = 100.0f;
    float maxAcceleration = 200.0f;
    float baseDPS = 15.0f;
    float attackRange = 80.0f;
    float attackCooldown = 0.8f;
    float influenceRadius = 60.0f;
    float massModifier = 1.0f;
    float restitution = 0.8f;
    float damageModifier = 1.0f;
};

/// Generic UE5 Character model wrapping a CoreEngine RTSUnit, UEGenericInventory,
/// 3D transform/animation state, timed actions, and TurnClashResolver attributes.
class UEGenericCharacter {
private:
    std::string characterId_;
    std::string displayName_;
    uint64_t boundEntityId_ = 0;
    int teamId_ = 1;

    int level_ = 1;
    int experience_ = 0;
    int xpToNextLevel_ = 100;

    float stamina_ = 100.0f;
    float maxStamina_ = 100.0f;
    float staminaRegenPerSecond_ = 8.0f;

    // TurnClashResolver & InteractionMatrix attributes (5-Precept Wheel & 5-Move Pentagon)
    int archetypeIndex_ = 0; ///< 0..4 precept/element index
    int momentum_ = 0;
    int initiative_ = 0;
    float readStat_ = 0.5f;
    float deceptionStat_ = 0.3f;

    UECharacterBaseStats baseStats_;
    UEGenericInventory inventory_;

    uint64_t activeTimedActionId_ = 0;
    float activeTimedActionProgress_ = 0.0f;
    UECharacterAnimState animState_ = UECharacterAnimState::Idle;
    UETransform transform3D_;
    float lastHeadingYawDegrees_ = 0.0f;

public:
    UEGenericCharacter(std::string characterId = "Hero_1",
                       std::string displayName = "Placeholder Hero",
                       int teamId = 1);

    // --- Identity & Binding ---
    const std::string& GetCharacterId() const { return characterId_; }
    const std::string& GetDisplayName() const { return displayName_; }
    void SetDisplayName(std::string name) { displayName_ = std::move(name); }
    int GetTeamId() const { return teamId_; }

    uint64_t GetBoundEntityId() const { return boundEntityId_; }
    void BindToUnit(RTSGameState& state, RTSUnit* unit);
    RTSUnit* ResolveUnit(RTSGameState& state) const;
    const RTSUnit* ResolveUnit(const RTSGameState& state) const;

    // --- Inventory & Equipment ---
    UEGenericInventory& GetInventory() { return inventory_; }
    const UEGenericInventory& GetInventory() const { return inventory_; }

    bool EquipItemAndRefresh(const std::string& itemId, RTSGameState& state);
    bool UnequipSlotAndRefresh(UEEquipmentSlot slot, RTSGameState& state);
    bool UseInventoryItem(const std::string& itemId, RTSGameState& state, DeploymentTimer* timer = nullptr);

    // --- Attributes & Leveling ---
    int GetLevel() const { return level_; }
    int GetExperience() const { return experience_; }
    int GetXpToNextLevel() const { return xpToNextLevel_; }
    bool AddExperience(int xpAmount, RTSGameState* state = nullptr);

    float GetStamina() const { return stamina_; }
    float GetMaxStamina() const { return maxStamina_; }
    void SetStamina(float s) { stamina_ = std::max(0.0f, std::min(maxStamina_, s)); }

    int GetArchetypeIndex() const { return archetypeIndex_; }
    void SetArchetypeIndex(int idx) { archetypeIndex_ = std::max(0, idx); }
    int GetMomentum() const { return momentum_; }
    void SetMomentum(int m) { momentum_ = std::max(0, m); }
    int GetInitiative() const { return initiative_; }
    float GetReadStat() const { return readStat_; }
    float GetDeceptionStat() const { return deceptionStat_; }
    void SetClashStats(float readStat, float deceptionStat) {
        readStat_ = readStat;
        deceptionStat_ = deceptionStat;
    }

    const UECharacterBaseStats& GetBaseStats() const { return baseStats_; }
    void SetBaseStats(const UECharacterBaseStats& stats, RTSUnit* unit = nullptr);

    /// Recomputes effective RTSUnit stats from baseStats_ + level scaling + equipped items.
    void RefreshEffectiveStats(RTSUnit* unit) const;

    // --- 3D Transform, Animation & Tick Sync ---
    const UETransform& GetTransform3D() const { return transform3D_; }
    UECharacterAnimState GetAnimState() const { return animState_; }
    const char* GetAnimStateName() const;
    uint64_t GetActiveTimedActionId() const { return activeTimedActionId_; }
    float GetActiveTimedActionProgress() const { return activeTimedActionProgress_; }

    /// Synchronize 3D transform, animation state, timed action progress, and cooldowns with RTSGameState.
    void SyncWithSimulation(RTSGameState& state, const UECoordinateBridge& bridge, float deltaTime);

    // --- Actions & Combat ---
    bool MoveToCore(RTSGameState& state, Vector2 destinationCore);
    bool MoveToUE(RTSGameState& state, const UEVec3& destinationUE, const UECoordinateBridge& bridge);

    bool SwapBehaviorState(RTSGameState& state, const std::string& behaviorKey);
    bool EjectBehaviorState(RTSGameState& state);

    uint64_t FireProjectileAt(RTSGameState& state, Vector2 targetCorePos,
                              float speed = 260.0f, float damage = 25.0f,
                              float radius = 6.0f, float lifetimeSeconds = 3.0f);

    /// Uses ProjectileSimulation::Intercept to lead a moving target unit automatically.
    uint64_t FireInterceptProjectileAtUnit(RTSGameState& state, const IGameUnit* targetUnit,
                                           float speed = 260.0f, float damage = 25.0f,
                                           float radius = 6.0f, float lifetimeSeconds = 3.0f);

    uint64_t StartTimedAbility(RTSGameState& state, uint32_t durationTicks, float staminaCost = 10.0f);
    bool CancelTimedAbility(RTSGameState& state);

    /// Resolve a tactical clash against another character using TurnClashResolver + InteractionMatrix.
    ClashResolutionResult ExecuteTurnClashWith(UEGenericCharacter& responder,
                                               int initiatorMoveType,
                                               int responderMoveType,
                                               RTSGameState& state,
                                               TelegraphResult* outTelegraph = nullptr);
};

} // namespace Unreal
} // namespace CoreEngine
