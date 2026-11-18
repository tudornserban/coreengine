#pragma once

#include "core/interfaces/IGameUnit.h"
#include "core/math/Vector2.h"

#include <cstdint>
#include <vector>

/// Traveling body. Games choose charge, cost, and firing policy; this is only motion.
struct Projectile {
    uint64_t id = 0;
    uint64_t ownerId = 0;
    int team = 0;
    Vector2 position;
    Vector2 velocity;
    float radius = 0.0f;
    float timeRemaining = 0.0f;
    float damage = 0.0f;
    bool hitAllies = false;
};

struct ProjectileImpact {
    uint64_t projectileId = 0;
    uint64_t targetId = 0;
    IGameUnit* target = nullptr;
    float time = 0.0f;
    Vector2 point;
};

struct InterceptSolution {
    bool hit = false;
    float time = 0.0f;
    Vector2 point;
};

/// Swept projectile step. Impacts are ordered by time, then projectile id, then target id.
/// A projectile hits only its earliest target. Expired projectiles are removed without an impact.
class ProjectileSimulation {
public:
    uint64_t Spawn(Projectile projectile);

    /// Advance by `deltaTime`. Applies `IUnitCombat::TakeDamage` when the projectile damage is positive.
    std::vector<ProjectileImpact> Advance(float deltaTime, const std::vector<IGameUnit*>& units);

    const std::vector<Projectile>& Projectiles() const { return projectiles_; }
    uint64_t NextId() const { return nextId_; }

    void Restore(std::vector<Projectile> projectiles, uint64_t nextId);

    /// Constant-speed intercept of a target moving at `targetVelocity`. Independent of firing policy.
    static InterceptSolution Intercept(Vector2 origin, float speed, Vector2 targetPosition, Vector2 targetVelocity);

private:
    std::vector<Projectile> projectiles_;
    uint64_t nextId_ = 1;
};
