#pragma once

/// Optional combat surface. Not part of IGameUnit.
/// `GetAttackCooldown() == 0` means continuous DPS. A positive cooldown is the
/// interval between discrete shots whose damage is `GetBaseDPS() * cooldown`.
class IUnitCombat {
public:
    virtual ~IUnitCombat() = default;

    virtual void TakeDamage(float amount) = 0;
    virtual bool IsDead() const = 0;

    virtual float GetAttackRange() const { return 0.0f; }
    virtual float GetBaseDPS() const { return 0.0f; }
    virtual float GetAttackCooldown() const { return 0.0f; }
};
