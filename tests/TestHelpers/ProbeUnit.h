#pragma once

#include "core/interfaces/IInfluenceEmitter.h"
#include "core/interfaces/IUnitCombat.h"
#include "tests/TestHelpers/MockGameUnit.h"

/// Test double with influence and combat surfaces on top of MockGameUnit.
class ProbeUnit : public MockGameUnit, public IInfluenceEmitter, public IUnitCombat {
public:
    using MockGameUnit::MockGameUnit;

    float GetInfluenceRadius() const override { return influenceRadius_; }
    void SetInfluenceRadius(float radius) override { influenceRadius_ = radius < 0.0f ? 0.0f : radius; }

    void TakeDamage(float amount) override {
        if (amount < 0.0f) {
            amount = 0.0f;
        }
        SetHP(GetCurrentHP() - amount);
    }
    bool IsDead() const override { return GetCurrentHP() <= 0.0f; }
    float GetAttackRange() const override { return attackRange_; }
    float GetBaseDPS() const override { return baseDps_; }
    float GetAttackCooldown() const override { return attackCooldown_; }

    void SetAttackRange(float range) { attackRange_ = range < 0.0f ? 0.0f : range; }
    void SetBaseDPS(float dps) { baseDps_ = dps < 0.0f ? 0.0f : dps; }
    void SetAttackCooldown(float cooldown) { attackCooldown_ = cooldown < 0.0f ? 0.0f : cooldown; }

private:
    float influenceRadius_ = 0.0f;
    float attackRange_ = 0.0f;
    float baseDps_ = 0.0f;
    float attackCooldown_ = 0.0f;
};
