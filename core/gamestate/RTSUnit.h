#pragma once

#include "../interfaces/IGameUnit.h"
#include "../interfaces/IUnitKinematics.h"
#include "../interfaces/IUnitAgent.h"
#include "../interfaces/IUnitBehaviorHost.h"
#include "../interfaces/IInfluenceEmitter.h"
#include "../interfaces/IUnitCombat.h"
#include "../ai/StateMachine.h"
#include "../math/Vector2.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>

/// How `SetMaxHP` treats the current hit points.
/// `ClampCurrent` keeps the current value and clamps it into `[0, newMax]`.
/// `PreserveRatio` scales current HP by `newMax / oldMax` (a zero old max clamps instead).
enum class MaxHpPolicy : uint8_t {
    ClampCurrent = 0,
    PreserveRatio = 1
};

/// Game-supplied presentation curve. CoreEngine does not encode a health-to-radius rule.
/// Arguments are current HP, maximum HP, and the collision radius.
using VisualRadiusCurve = std::function<float(float, float, float)>;

class RTSUnit : public IGameUnit,
                 public IUnitKinematics,
                 public IUnitAgent,
                 public IUnitBehaviorHost,
                 public IInfluenceEmitter,
                 public IUnitCombat {
protected:
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;

    float maxSpeed;
    float maxAcceleration;
    float radius;
    int team;
    bool selected;

    // HP and Mass
    float currentHP;
    float maxHP;
    float baseMass;
    float massModifier;

    // Collision properties
    float restitution;
    float positionCorrection;
    float damageModifier;

    float influenceRadius;
    float attackRange;
    float baseDps;
    float attackCooldown;

    StateMachine stateMachine;

    // Debug mode
    bool debugMode;
    Vector2 debugDestination;

    bool movementParticipation;
    bool actionParticipation;
    bool collisionParticipation;
    bool deathLatched;
    uint64_t entityId;
    VisualRadiusCurve visualRadiusCurve;

public:
    RTSUnit(Vector2 startPos, int teamId, float unitRadius = 10.0f, float unitMaxSpeed = 100.0f);
    virtual ~RTSUnit() = default;

    // IGameUnit implementation
    void Update(float deltaTime) override;
    Vector2 GetPosition() const override { return position; }
    Vector2 GetVelocity() const override { return velocity; }
    void ApplyAcceleration(Vector2 accel, float deltaTime) override;
    void ApplyPropulsion(Vector2 accel, float deltaTime) override;
    void ApplyImpulse(Vector2 momentum) override;
    void ApplyVelocityChange(Vector2 deltaV) override;
    void IntegrateMotion(float deltaTime) override;
    MotionIntegration GetMotionIntegration() const override { return MotionIntegration::SplitTick; }
    float GetMaxAcceleration() const override { return maxAcceleration; }

    bool IsSelected() const override { return selected; }
    void SetSelected(bool sel) override { selected = sel; }

    int GetTeam() const override { return team; }
    float GetRadius() const override { return radius; }
    /// Presentation radius. The default curve is `collisionRadius * hp / maxHp`
    /// (and the collision radius when max HP is 0). Games replace it with `SetVisualRadiusCurve`.
    /// Results below 0 are clamped to 0. This value is not the collision radius.
    float GetVisualRadius() const;
    float GetMaxSpeed() const override { return maxSpeed; }

    uint64_t GetEntityId() const override { return entityId; }
    void SetEntityId(uint64_t id) override { entityId = id; }
    bool ParticipatesInActions() const override { return actionParticipation; }
    bool ParticipatesInMovement() const override { return movementParticipation; }
    bool ParticipatesInCollision() const override { return collisionParticipation; }
    bool ConsumeDeathEvent() override;

    void SetMovementParticipation(bool enabled) { movementParticipation = enabled; }
    void SetActionParticipation(bool enabled) { actionParticipation = enabled; }
    void SetCollisionParticipation(bool enabled) { collisionParticipation = enabled; }
    bool IsDeathLatched() const { return deathLatched; }

    void SetDebugDestination(Vector2 target) override {
        debugDestination = target;
        debugMode = true;
    }
    bool IsInDebugMode() const override { return debugMode; }
    void ClearDebugMode() { debugMode = false; }

    // State machine access
    StateMachine& GetStateMachine() { return stateMachine; }
    const StateMachine& GetStateMachine() const { return stateMachine; }

    // Setters (IUnitKinematics)
    void SetPosition(Vector2 pos) override { position = pos; }
    void SetVelocity(Vector2 vel) override { velocity = vel; }
    void SetMaxSpeed(float speed) { maxSpeed = speed; }
    /// Negative values are rejected and the previous limit is kept. Zero disables propulsion.
    void SetMaxAcceleration(float limit);
    /// Negative radii are clamped to 0. Collision queries read this immediately.
    void SetRadius(float collisionRadius);
    void SetVisualRadiusCurve(VisualRadiusCurve curve) { visualRadiusCurve = std::move(curve); }

    // IUnitAgent (command surface for FSM / behavior parameters)
    void SetAgentState(const std::string& stateName) override;
    void SetAgentParameter(const std::string& paramName, float value) override;

    // IUnitBehaviorHost
    IBehavior* GetCurrentBehavior() override;

    // HP and Mass
    float GetCurrentHP() const override { return currentHP; }
    float GetMaxHP() const override { return maxHP; }
    float GetBaseMass() const override { return baseMass; }
    float GetMassModifier() const override { return massModifier; }
    float CalculateMass() const override;
    void SetHP(float hp) { currentHP = std::max(0.0f, std::min(maxHP, hp)); }
    /// See `MaxHpPolicy`. A negative maximum is rejected. Zero maximum clamps current HP to 0
    /// and does not divide by zero in mass or radius helpers.
    void SetMaxHP(float newMax, MaxHpPolicy policy = MaxHpPolicy::ClampCurrent);

    float GetInfluenceRadius() const override { return influenceRadius; }
    void SetInfluenceRadius(float radius) override { influenceRadius = std::max(0.0f, radius); }

    void TakeDamage(float amount) override { currentHP = std::max(0.0f, currentHP - amount); }
    bool IsDead() const override { return currentHP <= 0.0f; }
    float GetAttackRange() const override { return attackRange; }
    float GetBaseDPS() const override { return baseDps; }
    float GetAttackCooldown() const override { return attackCooldown; }
    void SetAttackRange(float range) { attackRange = std::max(0.0f, range); }
    void SetBaseDPS(float dps) { baseDps = std::max(0.0f, dps); }
    void SetAttackCooldown(float cooldown) { attackCooldown = std::max(0.0f, cooldown); }

    // Collision properties
    float GetRestitution() const override { return restitution; }
    float GetPositionCorrection() const override { return positionCorrection; }
    float GetDamageModifier() const override { return damageModifier; }
    void SetRestitution(float rest) { restitution = rest; }
    void SetMassModifier(float mod) { massModifier = mod; }
    void SetBaseMass(float mass) {
        if (mass > 0.0f) {
            baseMass = mass;
        }
    }
    void RestoreDeathLatch(bool latched) { deathLatched = latched; }
    void SetDamageModifier(float mod) { damageModifier = mod; }
    void SetPositionCorrection(float corr) { positionCorrection = corr; }
};
