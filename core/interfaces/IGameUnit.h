#pragma once

#include "../math/Vector2.h"

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
