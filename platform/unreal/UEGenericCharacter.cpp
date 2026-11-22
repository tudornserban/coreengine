#include "platform/unreal/UEGenericCharacter.h"
#include "core/ai/BehaviorFactory.h"

#include <algorithm>
#include <cmath>

namespace CoreEngine {
namespace Unreal {

UEGenericCharacter::UEGenericCharacter(std::string characterId,
                                       std::string displayName,
                                       int teamId)
    : characterId_(std::move(characterId))
    , displayName_(std::move(displayName))
    , teamId_(teamId)
    , inventory_(16, 120.0f) {}

void UEGenericCharacter::BindToUnit(RTSGameState& state, RTSUnit* unit) {
    if (!unit) {
        boundEntityId_ = 0;
        return;
    }
    boundEntityId_ = state.GetEntityId(unit);
    teamId_ = unit->GetTeam();

    baseStats_.maxHP = unit->GetMaxHP();
    baseStats_.maxSpeed = unit->GetMaxSpeed();
    baseStats_.maxAcceleration = unit->GetMaxAcceleration();
    baseStats_.baseDPS = unit->GetBaseDPS();
    baseStats_.attackRange = unit->GetAttackRange();
    baseStats_.attackCooldown = unit->GetAttackCooldown();
    baseStats_.influenceRadius = unit->GetInfluenceRadius();
    baseStats_.massModifier = unit->GetMassModifier();
    baseStats_.restitution = unit->GetRestitution();
    baseStats_.damageModifier = unit->GetDamageModifier();

    RefreshEffectiveStats(unit);
}

RTSUnit* UEGenericCharacter::ResolveUnit(RTSGameState& state) const {
    if (boundEntityId_ == 0) {
        return nullptr;
    }
    for (IGameUnit* u : state.GetAllUnits()) {
        if (u && state.GetEntityId(u) == boundEntityId_) {
            return dynamic_cast<RTSUnit*>(u);
        }
    }
    return nullptr;
}

const RTSUnit* UEGenericCharacter::ResolveUnit(const RTSGameState& state) const {
    if (boundEntityId_ == 0) {
        return nullptr;
    }
    for (IGameUnit* u : state.GetAllUnits()) {
        if (u && state.GetEntityId(u) == boundEntityId_) {
            return dynamic_cast<const RTSUnit*>(u);
        }
    }
    return nullptr;
}

bool UEGenericCharacter::EquipItemAndRefresh(const std::string& itemId, RTSGameState& state) {
    RTSUnit* unit = ResolveUnit(state);
    if (!inventory_.EquipByItemId(itemId, unit)) {
        return false;
    }
    if (unit) {
        RefreshEffectiveStats(unit);
    }
    return true;
}

bool UEGenericCharacter::UnequipSlotAndRefresh(UEEquipmentSlot slot, RTSGameState& state) {
    if (!inventory_.UnequipSlot(slot)) {
        return false;
    }
    if (RTSUnit* unit = ResolveUnit(state)) {
        RefreshEffectiveStats(unit);
    }
    return true;
}

bool UEGenericCharacter::UseInventoryItem(const std::string& itemId, RTSGameState& state,
                                          DeploymentTimer* timer) {
    RTSUnit* unit = ResolveUnit(state);
    uint64_t startedActionId = 0;
    if (!inventory_.UseItemOnUnit(itemId, unit, &state, timer, &startedActionId)) {
        return false;
    }
    if (startedActionId != 0) {
        activeTimedActionId_ = startedActionId;
        activeTimedActionProgress_ = 0.0f;
    }
    return true;
}

bool UEGenericCharacter::AddExperience(int xpAmount, RTSGameState* state) {
    if (xpAmount <= 0) {
        return false;
    }
    experience_ += xpAmount;
    bool leveledUp = false;
    while (experience_ >= xpToNextLevel_) {
        experience_ -= xpToNextLevel_;
        level_ += 1;
        xpToNextLevel_ = static_cast<int>(std::round(static_cast<float>(xpToNextLevel_) * 1.35f));
        maxStamina_ += 10.0f;
        stamina_ = maxStamina_;
        leveledUp = true;
    }
    if (leveledUp && state) {
        if (RTSUnit* unit = ResolveUnit(*state)) {
            RefreshEffectiveStats(unit);
        }
    }
    return leveledUp;
}

void UEGenericCharacter::SetBaseStats(const UECharacterBaseStats& stats, RTSUnit* unit) {
    baseStats_ = stats;
    if (unit) {
        RefreshEffectiveStats(unit);
    }
}

void UEGenericCharacter::RefreshEffectiveStats(RTSUnit* unit) const {
    if (!unit) {
        return;
    }
    const UEItemStatModifiers eq = inventory_.ComputeTotalEquipmentModifiers();
    const float levelBonus = static_cast<float>(std::max(0, level_ - 1));

    const float effMaxHp = std::max(1.0f, baseStats_.maxHP + levelBonus * 12.0f + eq.maxHpDelta);
    const float effMaxSpeed = std::max(0.0f, baseStats_.maxSpeed + levelBonus * 3.0f + eq.maxSpeedDelta);
    const float effMaxAccel = std::max(0.0f, baseStats_.maxAcceleration + levelBonus * 5.0f + eq.maxAccelerationDelta);
    const float effDps = std::max(0.0f, baseStats_.baseDPS + levelBonus * 2.5f + eq.baseDpsDelta);
    const float effRange = std::max(0.0f, baseStats_.attackRange + eq.attackRangeDelta);
    const float effInfluence = std::max(0.0f, baseStats_.influenceRadius + eq.influenceRadiusDelta);
    const float effMassMod = std::max(0.1f, baseStats_.massModifier + eq.massModifierDelta);
    const float effRestitution = std::max(0.0f, baseStats_.restitution + eq.restitutionDelta);
    const float effDamageMod = std::max(0.0f, baseStats_.damageModifier + eq.damageModifierDelta);

    unit->SetMaxHP(effMaxHp, MaxHpPolicy::PreserveRatio);
    unit->SetMaxSpeed(effMaxSpeed);
    unit->SetMaxAcceleration(effMaxAccel);
    unit->SetBaseDPS(effDps);
    unit->SetAttackRange(effRange);
    unit->SetAttackCooldown(baseStats_.attackCooldown);
    unit->SetInfluenceRadius(effInfluence);
    unit->SetMassModifier(effMassMod);
    unit->SetRestitution(effRestitution);
    unit->SetDamageModifier(effDamageMod);
}

const char* UEGenericCharacter::GetAnimStateName() const {
    switch (animState_) {
        case UECharacterAnimState::Idle: return "Idle";
        case UECharacterAnimState::Moving: return "Moving";
        case UECharacterAnimState::Attacking: return "Attacking";
        case UECharacterAnimState::CastingTimedAction: return "CastingTimedAction";
        case UECharacterAnimState::HitImpulse: return "HitImpulse";
        case UECharacterAnimState::Dead: return "Dead";
        default: return "Idle";
    }
}

void UEGenericCharacter::SyncWithSimulation(RTSGameState& state,
                                            const UECoordinateBridge& bridge,
                                            float deltaTime) {
    inventory_.UpdateCooldowns(deltaTime);
    if (deltaTime > 0.0f) {
        stamina_ = std::min(maxStamina_, stamina_ + staminaRegenPerSecond_ * deltaTime);
    }

    RTSUnit* unit = ResolveUnit(state);
    if (!unit) {
        animState_ = UECharacterAnimState::Dead;
        return;
    }

    const float capsuleHalfHeight = std::max(bridge.CoreToUEDistance(unit->GetVisualRadius()) * 1.4f,
                                             bridge.CoreToUEDistance(12.0f));
    lastHeadingYawDegrees_ = bridge.ComputeHeadingYawDegrees(unit->GetVelocity(), lastHeadingYawDegrees_);
    transform3D_ = bridge.BuildUnitTransform(
        unit->GetPosition(), unit->GetVelocity(), capsuleHalfHeight, lastHeadingYawDegrees_);

    // Timed action tracking
    bool hasRunningTimedAction = false;
    if (activeTimedActionId_ != 0) {
        const TimedAction* act = state.GetTimedActions().Find(activeTimedActionId_);
        if (act && act->status == TimedActionStatus::Running) {
            hasRunningTimedAction = true;
            activeTimedActionProgress_ = TimedActionClock::Progress(*act);
        } else {
            activeTimedActionProgress_ = 1.0f;
            activeTimedActionId_ = 0;
        }
    }

    // Check diagnostics for impulse or active combat
    bool hadImpulseThisTick = false;
    if (state.DiagnosticsEnabled()) {
        for (const auto& rec : state.GetDiagnostics()) {
            if (rec.entityId == boundEntityId_) {
                if (rec.impulse.LengthSquared() > 1.0f) {
                    hadImpulseThisTick = true;
                }
                break;
            }
        }
    }

    if (unit->IsDead()) {
        animState_ = UECharacterAnimState::Dead;
    } else if (hadImpulseThisTick) {
        animState_ = UECharacterAnimState::HitImpulse;
    } else if (hasRunningTimedAction) {
        animState_ = UECharacterAnimState::CastingTimedAction;
    } else if (unit->GetVelocity().LengthSquared() > 4.0f) {
        animState_ = UECharacterAnimState::Moving;
    } else {
        animState_ = UECharacterAnimState::Idle;
    }
}

bool UEGenericCharacter::MoveToCore(RTSGameState& state, Vector2 destinationCore) {
    RTSUnit* unit = ResolveUnit(state);
    if (!unit || unit->IsDead()) {
        return false;
    }
    state.GetCommandSystem().MoveUnitsTo({unit}, destinationCore);
    return true;
}

bool UEGenericCharacter::MoveToUE(RTSGameState& state, const UEVec3& destinationUE,
                                  const UECoordinateBridge& bridge) {
    return MoveToCore(state, bridge.UEToCorePosition(destinationUE));
}

bool UEGenericCharacter::SwapBehaviorState(RTSGameState& state, const std::string& behaviorKey) {
    RTSUnit* unit = ResolveUnit(state);
    if (!unit || behaviorKey.empty()) {
        return false;
    }
    StateMachine& fsm = unit->GetStateMachine();
    if (fsm.GetStates().find(behaviorKey) == fsm.GetStates().end()) {
        auto created = BehaviorFactory::Create(behaviorKey);
        if (!created) {
            return false;
        }
        fsm.AddState(behaviorKey, std::move(created));
    }
    fsm.SetState(behaviorKey);
    unit->ClearDebugMode();
    return true;
}

bool UEGenericCharacter::EjectBehaviorState(RTSGameState& state) {
    RTSUnit* unit = ResolveUnit(state);
    if (!unit) {
        return false;
    }
    unit->GetStateMachine().ClearCurrentState();
    return true;
}

uint64_t UEGenericCharacter::FireProjectileAt(RTSGameState& state, Vector2 targetCorePos,
                                              float speed, float damage,
                                              float radius, float lifetimeSeconds) {
    RTSUnit* unit = ResolveUnit(state);
    if (!unit || unit->IsDead() || speed <= 0.0f) {
        return 0;
    }
    const Vector2 origin = unit->GetPosition();
    Vector2 dir = (targetCorePos - origin).Normalized();
    if (dir.LengthSquared() <= 1e-6f) {
        dir = Vector2(1.0f, 0.0f);
    }

    Projectile p;
    p.ownerId = boundEntityId_;
    p.team = unit->GetTeam();
    p.position = origin;
    p.velocity = dir * speed;
    p.radius = radius;
    p.timeRemaining = lifetimeSeconds;
    p.damage = damage;
    p.hitAllies = false;

    animState_ = UECharacterAnimState::Attacking;
    return state.GetProjectiles().Spawn(p);
}

uint64_t UEGenericCharacter::FireInterceptProjectileAtUnit(RTSGameState& state,
                                                           const IGameUnit* targetUnit,
                                                           float speed, float damage,
                                                           float radius, float lifetimeSeconds) {
    RTSUnit* unit = ResolveUnit(state);
    if (!unit || !targetUnit || unit->IsDead() || speed <= 0.0f) {
        return 0;
    }
    const InterceptSolution sol = ProjectileSimulation::Intercept(
        unit->GetPosition(), speed, targetUnit->GetPosition(), targetUnit->GetVelocity());
    const Vector2 aimPoint = sol.hit ? sol.point : targetUnit->GetPosition();
    return FireProjectileAt(state, aimPoint, speed, damage, radius, lifetimeSeconds);
}

uint64_t UEGenericCharacter::StartTimedAbility(RTSGameState& state, uint32_t durationTicks,
                                               float staminaCost) {
    RTSUnit* unit = ResolveUnit(state);
    if (!unit || unit->IsDead() || durationTicks == 0 || stamina_ < staminaCost) {
        return 0;
    }
    stamina_ -= staminaCost;
    activeTimedActionId_ = state.GetTimedActions().Start(boundEntityId_, durationTicks);
    activeTimedActionProgress_ = 0.0f;
    animState_ = UECharacterAnimState::CastingTimedAction;
    return activeTimedActionId_;
}

bool UEGenericCharacter::CancelTimedAbility(RTSGameState& state) {
    if (activeTimedActionId_ == 0) {
        return false;
    }
    const bool ok = state.GetTimedActions().Cancel(activeTimedActionId_);
    activeTimedActionId_ = 0;
    activeTimedActionProgress_ = 0.0f;
    return ok;
}

ClashResolutionResult UEGenericCharacter::ExecuteTurnClashWith(UEGenericCharacter& responder,
                                                               int initiatorMoveType,
                                                               int responderMoveType,
                                                               RTSGameState& state,
                                                               TelegraphResult* outTelegraph) {
    const InteractionMatrix pentagon = InteractionMatrix::CreateFiveWayPentagon();
    const InteractionMatrix wheel = InteractionMatrix::CreateFivePreceptWheel();

    if (outTelegraph) {
        *outTelegraph = TurnClashResolver::ComputeTelegraph(
            initiatorMoveType, pentagon.Dimension(),
            responder.GetReadStat(), deceptionStat_, state.GetRNG());
    }

    RTSUnit* initUnit = ResolveUnit(state);
    RTSUnit* respUnit = responder.ResolveUnit(state);

    ClashMoveInput initMove;
    initMove.moveType = initiatorMoveType;
    initMove.baseDamage = initUnit ? std::max(10.0f, initUnit->GetBaseDPS()) : baseStats_.baseDPS;
    initMove.staminaCost = 12.0f;
    initMove.momentumGain = 1;
    initMove.initiativeShift = 1;
    initMove.deceptionBonus = deceptionStat_;

    ClashMoveInput respMove;
    respMove.moveType = responderMoveType;
    respMove.baseDamage = respUnit ? std::max(10.0f, respUnit->GetBaseDPS()) : responder.GetBaseStats().baseDPS;
    respMove.staminaCost = 10.0f;
    respMove.momentumGain = 1;
    respMove.initiativeShift = 1;
    respMove.deceptionBonus = responder.GetDeceptionStat();

    const ClashResolutionResult res = TurnClashResolver::ResolveClash(
        initMove, respMove,
        archetypeIndex_, responder.GetArchetypeIndex(),
        momentum_, responder.GetMomentum(),
        pentagon, wheel);

    stamina_ = std::max(0.0f, stamina_ - res.initiatorStaminaSpent);
    responder.SetStamina(responder.GetStamina() - res.responderStaminaSpent);
    momentum_ = std::max(0, momentum_ + res.initiatorMomentumDelta);
    responder.SetMomentum(responder.GetMomentum() + res.responderMomentumDelta);
    initiative_ += res.netInitiativeShift;
    responder.initiative_ -= res.netInitiativeShift;

    if (initUnit && res.damageToInitiator > 0.0f) {
        initUnit->TakeDamage(res.damageToInitiator);
    }
    if (respUnit && res.damageToResponder > 0.0f) {
        respUnit->TakeDamage(res.damageToResponder);
    }

    animState_ = UECharacterAnimState::Attacking;
    responder.animState_ = UECharacterAnimState::Attacking;
    return res;
}

} // namespace Unreal
} // namespace CoreEngine
