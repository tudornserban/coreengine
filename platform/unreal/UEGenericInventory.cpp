#include "platform/unreal/UEGenericInventory.h"
#include "core/ai/BehaviorFactory.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace CoreEngine {
namespace Unreal {

UEGenericInventory::UEGenericInventory(int maxSlots, float maxWeight)
    : maxSlots_(std::max(1, maxSlots)), maxWeight_(std::max(1.0f, maxWeight)) {
    slots_.resize(static_cast<std::size_t>(maxSlots_));
    for (int i = 0; i < maxSlots_; ++i) {
        slots_[static_cast<std::size_t>(i)].slotIndex = i;
    }
    RegisterDefaultCatalog();
}

void UEGenericInventory::RegisterItemDefinition(const UEItemDefinition& def) {
    if (!def.itemId.empty()) {
        catalog_[def.itemId] = def;
    }
}

const UEItemDefinition* UEGenericInventory::FindDefinition(const std::string& itemId) const {
    auto it = catalog_.find(itemId);
    return (it != catalog_.end()) ? &it->second : nullptr;
}

void UEGenericInventory::RegisterDefaultCatalog() {
    BehaviorFactory::RegisterBuiltins();

    // 1. Weapons & Armor
    {
        UEItemDefinition blade;
        blade.itemId = "weapon_plasma_blade";
        blade.displayName = "Plasma Blade";
        blade.description = "Increases attack DPS and close-quarters influence radius.";
        blade.category = UEItemCategory::Weapon;
        blade.equipSlot = UEEquipmentSlot::MainHand;
        blade.maxStack = 1;
        blade.unitWeight = 4.0f;
        blade.modifiers.baseDpsDelta = 18.0f;
        blade.modifiers.attackRangeDelta = 15.0f;
        blade.modifiers.influenceRadiusDelta = 10.0f;
        RegisterItemDefinition(blade);
    }
    {
        UEItemDefinition lance;
        lance.itemId = "weapon_rail_lance";
        lance.displayName = "Long-Range Rail Lance";
        lance.description = "Long-range precision weapon boosting attack range and DPS.";
        lance.category = UEItemCategory::Weapon;
        lance.equipSlot = UEEquipmentSlot::MainHand;
        lance.maxStack = 1;
        lance.unitWeight = 6.0f;
        lance.modifiers.baseDpsDelta = 14.0f;
        lance.modifiers.attackRangeDelta = 60.0f;
        RegisterItemDefinition(lance);
    }
    {
        UEItemDefinition plate;
        plate.itemId = "armor_aegis_plate";
        plate.displayName = "Aegis Heavy Plate";
        plate.description = "Boosts Max HP and mass while slightly reducing top speed.";
        plate.category = UEItemCategory::Armor;
        plate.equipSlot = UEEquipmentSlot::Armor;
        plate.maxStack = 1;
        plate.unitWeight = 10.0f;
        plate.modifiers.maxHpDelta = 60.0f;
        plate.modifiers.massModifierDelta = 0.5f;
        plate.modifiers.maxSpeedDelta = -10.0f;
        RegisterItemDefinition(plate);
    }
    {
        UEItemDefinition thrusters;
        thrusters.itemId = "acc_vector_thrusters";
        thrusters.displayName = "Vector Thrusters";
        thrusters.description = "Increases maximum speed and propulsion acceleration.";
        thrusters.category = UEItemCategory::Accessory;
        thrusters.equipSlot = UEEquipmentSlot::Accessory;
        thrusters.maxStack = 1;
        thrusters.unitWeight = 2.5f;
        thrusters.modifiers.maxSpeedDelta = 40.0f;
        thrusters.modifiers.maxAccelerationDelta = 80.0f;
        RegisterItemDefinition(thrusters);
    }

    // 2. Consumables
    {
        UEItemDefinition medkit;
        medkit.itemId = "consumable_nanite_repair";
        medkit.displayName = "Nanite Repair Kit";
        medkit.description = "Instantly restores 45 HP and runs a 15-tick calibration action.";
        medkit.category = UEItemCategory::Consumable;
        medkit.maxStack = 5;
        medkit.unitWeight = 1.0f;
        medkit.cooldownSeconds = 2.0f;
        medkit.modifiers.instantHealAmount = 45.0f;
        medkit.modifiers.timedActionDurationTicks = 15;
        RegisterItemDefinition(medkit);
    }
    {
        UEItemDefinition chargeCell;
        chargeCell.itemId = "consumable_deploy_beacon";
        chargeCell.displayName = "Deployment Charge Cell";
        chargeCell.description = "Grants +1 immediate unit deployment charge to the team.";
        chargeCell.category = UEItemCategory::Consumable;
        chargeCell.maxStack = 3;
        chargeCell.unitWeight = 1.5f;
        chargeCell.cooldownSeconds = 1.0f;
        chargeCell.modifiers.grantDeploymentCharges = 1;
        RegisterItemDefinition(chargeCell);
    }

    // 3. Behavior Cartridges ("Magic Dust" / AI Behavior modules for every built-in behavior)
    struct CartridgeSpec {
        const char* id;
        const char* name;
        const char* behaviorKey;
        const char* desc;
    };
    const CartridgeSpec specs[] = {
        {"cartridge_seek", "Seek Dust Cartridge", "Seek", "Swaps unit AI state to Seek."},
        {"cartridge_flee", "Flee Dust Cartridge", "Flee", "Swaps unit AI state to Flee."},
        {"cartridge_wander", "Wander Dust Cartridge", "Wander", "Swaps unit AI state to Wander."},
        {"cartridge_cohesion", "Cohesion Dust Cartridge", "Cohesion", "Swaps unit AI state to Cohesion."},
        {"cartridge_separation", "Separation Dust Cartridge", "Separation", "Swaps unit AI state to Separation."},
        {"cartridge_frontline", "Tactical Frontline Cartridge", "TacticalFrontline", "Swaps unit AI state to TacticalFrontline."}
    };
    for (const auto& s : specs) {
        UEItemDefinition cart;
        cart.itemId = s.id;
        cart.displayName = s.name;
        cart.description = s.desc;
        cart.category = UEItemCategory::BehaviorCartridge;
        cart.equipSlot = UEEquipmentSlot::BehaviorCartridge;
        cart.maxStack = 1;
        cart.unitWeight = 0.5f;
        cart.modifiers.behaviorStateOverride = s.behaviorKey;
        RegisterItemDefinition(cart);
    }
}

int UEGenericInventory::GetUsedSlotCount() const {
    int count = 0;
    for (const auto& s : slots_) {
        if (!s.IsEmpty()) {
            ++count;
        }
    }
    return count;
}

float UEGenericInventory::GetCurrentWeight() const {
    float total = 0.0f;
    for (const auto& s : slots_) {
        total += s.TotalWeight();
    }
    for (const auto& kv : equipped_) {
        total += kv.second.unitWeight;
    }
    return total;
}

const UEInventorySlot* UEGenericInventory::GetSlot(int slotIndex) const {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size())) {
        return nullptr;
    }
    return &slots_[static_cast<std::size_t>(slotIndex)];
}

int UEGenericInventory::FindSlotByItemId(const std::string& itemId) const {
    for (const auto& s : slots_) {
        if (!s.IsEmpty() && s.definition.itemId == itemId) {
            return s.slotIndex;
        }
    }
    return -1;
}

int UEGenericInventory::GetItemCount(const std::string& itemId) const {
    int total = 0;
    for (const auto& s : slots_) {
        if (!s.IsEmpty() && s.definition.itemId == itemId) {
            total += s.quantity;
        }
    }
    return total;
}

bool UEGenericInventory::HasItem(const std::string& itemId, int minQuantity) const {
    return GetItemCount(itemId) >= minQuantity;
}

int UEGenericInventory::AddItem(const UEItemDefinition& def, int quantity) {
    if (def.itemId.empty() || quantity <= 0) {
        return 0;
    }
    RegisterItemDefinition(def);

    const float unitW = std::max(0.0f, def.unitWeight);
    const int maxStack = std::max(1, def.maxStack);
    int remaining = quantity;
    int added = 0;

    auto canFitOne = [&]() -> bool {
        if (unitW <= 0.0f) {
            return true;
        }
        return (GetCurrentWeight() + unitW) <= (maxWeight_ + 1e-4f);
    };

    // First fill existing partial stacks
    for (auto& s : slots_) {
        if (remaining <= 0) {
            break;
        }
        if (!s.IsEmpty() && s.definition.itemId == def.itemId && s.quantity < maxStack) {
            while (remaining > 0 && s.quantity < maxStack && canFitOne()) {
                s.quantity += 1;
                remaining -= 1;
                added += 1;
            }
        }
    }

    // Then fill empty slots
    for (auto& s : slots_) {
        if (remaining <= 0) {
            break;
        }
        if (s.IsEmpty()) {
            if (!canFitOne()) {
                break;
            }
            s.definition = def;
            s.quantity = 0;
            s.cooldownRemaining = 0.0f;
            while (remaining > 0 && s.quantity < maxStack && canFitOne()) {
                s.quantity += 1;
                remaining -= 1;
                added += 1;
            }
            if (s.quantity == 0) {
                s.definition = UEItemDefinition{};
            }
        }
    }

    return added;
}

int UEGenericInventory::AddItemById(const std::string& itemId, int quantity) {
    const UEItemDefinition* def = FindDefinition(itemId);
    if (!def) {
        return 0;
    }
    return AddItem(*def, quantity);
}

bool UEGenericInventory::RemoveItem(const std::string& itemId, int quantity) {
    if (quantity <= 0 || GetItemCount(itemId) < quantity) {
        return false;
    }
    int toRemove = quantity;
    for (auto& s : slots_) {
        if (toRemove <= 0) {
            break;
        }
        if (!s.IsEmpty() && s.definition.itemId == itemId) {
            const int take = std::min(s.quantity, toRemove);
            s.quantity -= take;
            toRemove -= take;
            if (s.quantity <= 0) {
                s.quantity = 0;
                s.definition = UEItemDefinition{};
                s.cooldownRemaining = 0.0f;
            }
        }
    }
    return true;
}

bool UEGenericInventory::RemoveFromSlot(int slotIndex, int quantity) {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size()) || quantity <= 0) {
        return false;
    }
    auto& s = slots_[static_cast<std::size_t>(slotIndex)];
    if (s.IsEmpty() || s.quantity < quantity) {
        return false;
    }
    s.quantity -= quantity;
    if (s.quantity <= 0) {
        s.quantity = 0;
        s.definition = UEItemDefinition{};
        s.cooldownRemaining = 0.0f;
    }
    return true;
}

void UEGenericInventory::Clear() {
    for (auto& s : slots_) {
        s.quantity = 0;
        s.definition = UEItemDefinition{};
        s.cooldownRemaining = 0.0f;
    }
    equipped_.clear();
}

bool UEGenericInventory::EquipFromSlot(int slotIndex, RTSUnit* targetUnit) {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size())) {
        return false;
    }
    auto& s = slots_[static_cast<std::size_t>(slotIndex)];
    if (s.IsEmpty() || s.definition.equipSlot == UEEquipmentSlot::None) {
        return false;
    }

    const UEItemDefinition itemToEquip = s.definition;
    const UEEquipmentSlot targetEquipSlot = itemToEquip.equipSlot;

    // Remove 1 from inventory slot first so weight remains balanced
    RemoveFromSlot(slotIndex, 1);

    // If something was already equipped in that slot, return it to inventory
    auto existingIt = equipped_.find(targetEquipSlot);
    if (existingIt != equipped_.end()) {
        const UEItemDefinition oldItem = existingIt->second;
        equipped_.erase(existingIt);
        AddItem(oldItem, 1);
    }

    equipped_[targetEquipSlot] = itemToEquip;

    // If equipping a BehaviorCartridge and targetUnit is provided, immediately swap its AI state
    if (targetUnit && !itemToEquip.modifiers.behaviorStateOverride.empty()) {
        const std::string& stateName = itemToEquip.modifiers.behaviorStateOverride;
        StateMachine& fsm = targetUnit->GetStateMachine();
        if (fsm.GetStates().find(stateName) == fsm.GetStates().end()) {
            auto created = BehaviorFactory::Create(stateName);
            if (created) {
                fsm.AddState(stateName, std::move(created));
            }
        }
        fsm.SetState(stateName);
        for (const auto& paramKv : itemToEquip.modifiers.behaviorParameterOverrides) {
            targetUnit->SetAgentParameter(paramKv.first, paramKv.second);
        }
    }

    return true;
}

bool UEGenericInventory::EquipByItemId(const std::string& itemId, RTSUnit* targetUnit) {
    const int slotIdx = FindSlotByItemId(itemId);
    if (slotIdx < 0) {
        return false;
    }
    return EquipFromSlot(slotIdx, targetUnit);
}

bool UEGenericInventory::UnequipSlot(UEEquipmentSlot equipSlot) {
    auto it = equipped_.find(equipSlot);
    if (it == equipped_.end()) {
        return false;
    }
    const UEItemDefinition item = it->second;
    equipped_.erase(it);
    if (AddItem(item, 1) == 0) {
        // Put back if no free slot
        equipped_[equipSlot] = item;
        return false;
    }
    return true;
}

const UEItemDefinition* UEGenericInventory::GetEquippedItem(UEEquipmentSlot equipSlot) const {
    auto it = equipped_.find(equipSlot);
    return (it != equipped_.end()) ? &it->second : nullptr;
}

UEItemStatModifiers UEGenericInventory::ComputeTotalEquipmentModifiers() const {
    UEItemStatModifiers total;
    for (const auto& kv : equipped_) {
        const auto& m = kv.second.modifiers;
        total.maxHpDelta += m.maxHpDelta;
        total.maxSpeedDelta += m.maxSpeedDelta;
        total.maxAccelerationDelta += m.maxAccelerationDelta;
        total.baseDpsDelta += m.baseDpsDelta;
        total.attackRangeDelta += m.attackRangeDelta;
        total.influenceRadiusDelta += m.influenceRadiusDelta;
        total.massModifierDelta += m.massModifierDelta;
        total.restitutionDelta += m.restitutionDelta;
        total.damageModifierDelta += m.damageModifierDelta;
        if (!m.behaviorStateOverride.empty()) {
            total.behaviorStateOverride = m.behaviorStateOverride;
        }
    }
    return total;
}

bool UEGenericInventory::UseItemOnUnit(const std::string& itemId,
                                       RTSUnit* targetUnit,
                                       RTSGameState* gameState,
                                       DeploymentTimer* deploymentTimer,
                                       uint64_t* outStartedTimedActionId) {
    const int slotIdx = FindSlotByItemId(itemId);
    if (slotIdx < 0) {
        return false;
    }
    auto& slot = slots_[static_cast<std::size_t>(slotIdx)];
    if (slot.IsEmpty() || slot.cooldownRemaining > 0.0f) {
        return false;
    }

    const UEItemDefinition def = slot.definition;

    // 1. Apply healing to target unit
    if (targetUnit && def.modifiers.instantHealAmount > 0.0f && !targetUnit->IsDead()) {
        targetUnit->SetHP(targetUnit->GetCurrentHP() + def.modifiers.instantHealAmount);
    }

    // 2. Apply behavior state swap (Magic Dust / Behavior Cartridge)
    if (targetUnit && !def.modifiers.behaviorStateOverride.empty()) {
        const std::string& stateName = def.modifiers.behaviorStateOverride;
        StateMachine& fsm = targetUnit->GetStateMachine();
        if (fsm.GetStates().find(stateName) == fsm.GetStates().end()) {
            auto created = BehaviorFactory::Create(stateName);
            if (created) {
                fsm.AddState(stateName, std::move(created));
            }
        }
        fsm.SetState(stateName);
        for (const auto& paramKv : def.modifiers.behaviorParameterOverrides) {
            targetUnit->SetAgentParameter(paramKv.first, paramKv.second);
        }
    }

    // 3. Grant deployment charges
    if (deploymentTimer && def.modifiers.grantDeploymentCharges > 0) {
        for (int i = 0; i < def.modifiers.grantDeploymentCharges; ++i) {
            deploymentTimer->GrantCharge();
        }
    }

    // 4. Start tick-counted TimedActionClock action if configured
    if (gameState && targetUnit && def.modifiers.timedActionDurationTicks > 0) {
        const uint64_t entityId = gameState->GetEntityId(targetUnit);
        const uint64_t actId = gameState->GetTimedActions().Start(
            entityId, def.modifiers.timedActionDurationTicks);
        if (outStartedTimedActionId) {
            *outStartedTimedActionId = actId;
        }
    }

    // 5. Consume stack if Consumable, or set cooldown
    if (def.category == UEItemCategory::Consumable) {
        RemoveFromSlot(slotIdx, 1);
        // Apply cooldown to remaining stack in that slot if any
        if (!slots_[static_cast<std::size_t>(slotIdx)].IsEmpty()) {
            slots_[static_cast<std::size_t>(slotIdx)].cooldownRemaining = def.cooldownSeconds;
        }
    } else {
        slot.cooldownRemaining = def.cooldownSeconds;
    }

    return true;
}

void UEGenericInventory::UpdateCooldowns(float deltaTime) {
    if (deltaTime <= 0.0f) {
        return;
    }
    for (auto& s : slots_) {
        if (s.cooldownRemaining > 0.0f) {
            s.cooldownRemaining = std::max(0.0f, s.cooldownRemaining - deltaTime);
        }
    }
}

std::vector<uint8_t> UEGenericInventory::CaptureSnapshotBytes() const {
    std::ostringstream oss;
    oss << "CE_INV_V1\n";
    oss << maxSlots_ << " " << maxWeight_ << "\n";

    int nonEmptySlots = 0;
    for (const auto& s : slots_) {
        if (!s.IsEmpty()) {
            ++nonEmptySlots;
        }
    }
    oss << nonEmptySlots << "\n";
    for (const auto& s : slots_) {
        if (!s.IsEmpty()) {
            oss << s.slotIndex << " " << s.definition.itemId << " "
                << s.quantity << " " << s.cooldownRemaining << "\n";
        }
    }

    oss << equipped_.size() << "\n";
    for (const auto& kv : equipped_) {
        oss << static_cast<int>(kv.first) << " " << kv.second.itemId << "\n";
    }

    const std::string str = oss.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

bool UEGenericInventory::ValidateSnapshotBytes(const std::vector<uint8_t>& bytes) const {
    if (bytes.empty()) {
        return false;
    }
    const std::string str(bytes.begin(), bytes.end());
    std::istringstream iss(str);
    std::string header;
    if (!(iss >> header) || header != "CE_INV_V1") {
        return false;
    }
    int snapMaxSlots = 0;
    float snapMaxWeight = 0.0f;
    if (!(iss >> snapMaxSlots >> snapMaxWeight) || snapMaxSlots <= 0 || snapMaxWeight <= 0.0f) {
        return false;
    }
    int slotCount = 0;
    if (!(iss >> slotCount) || slotCount < 0 || slotCount > snapMaxSlots) {
        return false;
    }
    for (int i = 0; i < slotCount; ++i) {
        int idx = 0;
        std::string itemId;
        int qty = 0;
        float cd = 0.0f;
        if (!(iss >> idx >> itemId >> qty >> cd)) {
            return false;
        }
        if (idx < 0 || idx >= snapMaxSlots || qty <= 0 || !FindDefinition(itemId)) {
            return false;
        }
    }
    int equipCount = 0;
    if (!(iss >> equipCount) || equipCount < 0 || equipCount > 16) {
        return false;
    }
    for (int i = 0; i < equipCount; ++i) {
        int slotEnum = 0;
        std::string itemId;
        if (!(iss >> slotEnum >> itemId) || !FindDefinition(itemId)) {
            return false;
        }
    }
    return true;
}

bool UEGenericInventory::RestoreSnapshotBytes(const std::vector<uint8_t>& bytes) {
    if (!ValidateSnapshotBytes(bytes)) {
        return false;
    }
    const std::string str(bytes.begin(), bytes.end());
    std::istringstream iss(str);
    std::string header;
    iss >> header;

    int snapMaxSlots = 0;
    float snapMaxWeight = 0.0f;
    iss >> snapMaxSlots >> snapMaxWeight;

    maxSlots_ = snapMaxSlots;
    maxWeight_ = snapMaxWeight;
    slots_.assign(static_cast<std::size_t>(maxSlots_), UEInventorySlot{});
    for (int i = 0; i < maxSlots_; ++i) {
        slots_[static_cast<std::size_t>(i)].slotIndex = i;
    }
    equipped_.clear();

    int slotCount = 0;
    iss >> slotCount;
    for (int i = 0; i < slotCount; ++i) {
        int idx = 0;
        std::string itemId;
        int qty = 0;
        float cd = 0.0f;
        iss >> idx >> itemId >> qty >> cd;
        const UEItemDefinition* def = FindDefinition(itemId);
        if (def) {
            auto& s = slots_[static_cast<std::size_t>(idx)];
            s.slotIndex = idx;
            s.definition = *def;
            s.quantity = qty;
            s.cooldownRemaining = cd;
        }
    }

    int equipCount = 0;
    iss >> equipCount;
    for (int i = 0; i < equipCount; ++i) {
        int slotEnum = 0;
        std::string itemId;
        iss >> slotEnum >> itemId;
        const UEItemDefinition* def = FindDefinition(itemId);
        if (def) {
            equipped_[static_cast<UEEquipmentSlot>(slotEnum)] = *def;
        }
    }
    return true;
}

} // namespace Unreal
} // namespace CoreEngine
