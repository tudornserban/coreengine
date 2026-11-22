#pragma once

#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/gameloop/DeploymentTimer.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace CoreEngine {
namespace Unreal {

enum class UEItemCategory : uint8_t {
    Consumable = 0,
    Weapon = 1,
    Armor = 2,
    Accessory = 3,
    BehaviorCartridge = 4, ///< e.g. RationalInsanity Magic Dust or AI behavior module
    Resource = 5,
    QuestOrKey = 6
};

enum class UEEquipmentSlot : uint8_t {
    None = 0,
    MainHand = 1,
    OffHand = 2,
    Armor = 3,
    Accessory = 4,
    BehaviorCartridge = 5
};

/// Stat & behavior modifiers applied by an equipped or consumed item to a CoreEngine RTSUnit.
struct UEItemStatModifiers {
    float maxHpDelta = 0.0f;
    float maxSpeedDelta = 0.0f;
    float maxAccelerationDelta = 0.0f;
    float baseDpsDelta = 0.0f;
    float attackRangeDelta = 0.0f;
    float influenceRadiusDelta = 0.0f;
    float massModifierDelta = 0.0f;
    float restitutionDelta = 0.0f;
    float damageModifierDelta = 0.0f;

    // On-use / cartridge effects
    float instantHealAmount = 0.0f;
    int grantDeploymentCharges = 0;
    uint32_t timedActionDurationTicks = 0;
    std::string behaviorStateOverride;
    std::map<std::string, float> behaviorParameterOverrides;
};

struct UEItemDefinition {
    std::string itemId;
    std::string displayName;
    std::string description;
    UEItemCategory category = UEItemCategory::Resource;
    UEEquipmentSlot equipSlot = UEEquipmentSlot::None;
    int maxStack = 1;
    float unitWeight = 1.0f;
    float cooldownSeconds = 0.0f;
    UEItemStatModifiers modifiers;
};

struct UEInventorySlot {
    int slotIndex = 0;
    UEItemDefinition definition;
    int quantity = 0;
    float cooldownRemaining = 0.0f;

    bool IsEmpty() const { return quantity <= 0 || definition.itemId.empty(); }
    float TotalWeight() const { return IsEmpty() ? 0.0f : definition.unitWeight * static_cast<float>(quantity); }
};

/// Generic, low-overhead Inventory & Equipment component designed for UE5 characters/units
/// backed by CoreEngine's RTSUnit and deterministic snapshot extensions.
class UEGenericInventory {
private:
    int maxSlots_ = 16;
    float maxWeight_ = 100.0f;
    std::vector<UEInventorySlot> slots_;
    std::map<UEEquipmentSlot, UEItemDefinition> equipped_;
    std::map<std::string, UEItemDefinition> catalog_;

public:
    explicit UEGenericInventory(int maxSlots = 16, float maxWeight = 100.0f);

    // --- Catalog Management ---
    void RegisterItemDefinition(const UEItemDefinition& def);
    const UEItemDefinition* FindDefinition(const std::string& itemId) const;
    const std::map<std::string, UEItemDefinition>& GetCatalog() const { return catalog_; }
    void RegisterDefaultCatalog();

    // --- Capacity & Queries ---
    int GetMaxSlots() const { return maxSlots_; }
    int GetUsedSlotCount() const;
    float GetMaxWeight() const { return maxWeight_; }
    void SetMaxWeight(float maxWeight) { if (maxWeight > 0.0f) maxWeight_ = maxWeight; }
    float GetCurrentWeight() const;

    const std::vector<UEInventorySlot>& GetSlots() const { return slots_; }
    const UEInventorySlot* GetSlot(int slotIndex) const;
    int FindSlotByItemId(const std::string& itemId) const;
    int GetItemCount(const std::string& itemId) const;
    bool HasItem(const std::string& itemId, int minQuantity = 1) const;

    // --- Item Mutation ---
    /// Add quantity of item by definition or catalog ID. Returns number of items actually added.
    int AddItem(const UEItemDefinition& def, int quantity = 1);
    int AddItemById(const std::string& itemId, int quantity = 1);

    /// Remove up to quantity of itemId across slots. Returns true if the full requested amount was removed.
    bool RemoveItem(const std::string& itemId, int quantity = 1);
    bool RemoveFromSlot(int slotIndex, int quantity = 1);
    void Clear();

    // --- Equipment Management ---
    bool EquipFromSlot(int slotIndex, RTSUnit* targetUnit = nullptr);
    bool EquipByItemId(const std::string& itemId, RTSUnit* targetUnit = nullptr);
    bool UnequipSlot(UEEquipmentSlot equipSlot);
    const UEItemDefinition* GetEquippedItem(UEEquipmentSlot equipSlot) const;
    const std::map<UEEquipmentSlot, UEItemDefinition>& GetEquippedItems() const { return equipped_; }

    /// Aggregate stat modifiers across all currently equipped items.
    UEItemStatModifiers ComputeTotalEquipmentModifiers() const;

    // --- Item Execution on CoreEngine RTSUnit / RTSGameState ---
    /// Use an item (Consumable or BehaviorCartridge) on a CoreEngine RTSUnit.
    /// Applies healing, behavior state swap (via BehaviorFactory & StateMachine),
    /// timed actions (via TimedActionClock), and deployment charge grants.
    bool UseItemOnUnit(const std::string& itemId,
                       RTSUnit* targetUnit,
                       RTSGameState* gameState = nullptr,
                       DeploymentTimer* deploymentTimer = nullptr,
                       uint64_t* outStartedTimedActionId = nullptr);

    void UpdateCooldowns(float deltaTime);

    // --- Deterministic Snapshot Extension (for RTSGameState::SetGameExtension) ---
    std::vector<uint8_t> CaptureSnapshotBytes() const;
    bool ValidateSnapshotBytes(const std::vector<uint8_t>& bytes) const;
    bool RestoreSnapshotBytes(const std::vector<uint8_t>& bytes);
};

} // namespace Unreal
} // namespace CoreEngine
