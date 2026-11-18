#pragma once

#include "../math/Vector2.h"
#include <cstdint>

/// How a unit consumes propulsion during `RTSGameState::Update`.
/// `CombinedInApplyAcceleration` keeps the legacy `ApplyAcceleration` call.
/// `SplitTick` applies propulsion, then impulses, then one position step.
enum class MotionIntegration : uint8_t {
    CombinedInApplyAcceleration = 0,
    SplitTick = 1
};

/// Pure-virtual contract for any unit in the simulation.
/// Implement this interface to create game-specific unit types
/// that work with the core AI, collision, and influence systems.
class IGameUnit {
public:
    virtual ~IGameUnit() = default;

    // --- Lifecycle ---
    virtual void Update(float deltaTime) = 0;

    // --- Spatial ---
    virtual Vector2 GetPosition() const = 0;
    virtual Vector2 GetVelocity() const = 0;
    virtual void ApplyAcceleration(Vector2 accel, float deltaTime) = 0;

    /// Propulsion changes velocity and enforces `GetMaxSpeed`. It does not move the unit.
    /// The default keeps legacy callers on `ApplyAcceleration`.
    virtual void ApplyPropulsion(Vector2 accel, float deltaTime) { ApplyAcceleration(accel, deltaTime); }

    /// Momentum impulse. `deltaV = momentum / max(mass, 1)`. No position change and no speed clamp.
    virtual void ApplyImpulse(Vector2 /*momentum*/) {}

    /// Direct velocity change. No position change and no speed clamp.
    /// Explosions use this because `Explosion::CalculateImpulse` is already mass-scaled.
    virtual void ApplyVelocityChange(Vector2 /*deltaV*/) {}

    /// One position step: `position += velocity * deltaTime`.
    virtual void IntegrateMotion(float /*deltaTime*/) {}

    virtual MotionIntegration GetMotionIntegration() const { return MotionIntegration::CombinedInApplyAcceleration; }

    /// Final combined steering acceleration cap. Propulsion uses this; impulses do not.
    virtual float GetMaxAcceleration() const { return 300.0f; }

    /// Stable simulation-local id. `0` means the hub has not assigned one.
    virtual uint64_t GetEntityId() const { return 0; }
    virtual void SetEntityId(uint64_t /*id*/) {}

    /// Action participation gates behavior and automatic transitions.
    /// Movement participation gates propulsion, impulses, and the position step (coasting).
    /// Collision participation gates the contact pass. Defaults keep every unit active.
    virtual bool ParticipatesInActions() const { return true; }
    virtual bool ParticipatesInMovement() const { return true; }
    virtual bool ParticipatesInCollision() const { return true; }

    /// Latch a death the first time HP is depleted. Later calls return false until HP is positive again.
    virtual bool ConsumeDeathEvent() { return false; }

    // --- Identity ---
    virtual int GetTeam() const = 0;
    virtual float GetRadius() const = 0;
    virtual float GetMaxSpeed() const = 0;

    // --- Selection (optional; return false if not supported) ---
    virtual bool IsSelected() const { return false; }
    virtual void SetSelected(bool /*selected*/) {}

    // --- Health / Mass ---
    virtual float GetCurrentHP() const = 0;
    virtual float GetMaxHP() const = 0;
    virtual float GetBaseMass() const { return 100.0f; }
    virtual float GetMassModifier() const { return 1.0f; }
    virtual float CalculateMass() const { return GetBaseMass() * GetMassModifier(); }

    // --- Collision properties ---
    virtual float GetRestitution() const { return 0.7f; }
    virtual float GetPositionCorrection() const { return 0.2f; }
    virtual float GetDamageModifier() const { return 1.0f; }

    // --- Debug ---
    virtual void SetDebugDestination(Vector2 /*target*/) {}
    virtual bool IsInDebugMode() const { return false; }
};
