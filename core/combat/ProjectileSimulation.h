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

// Value event safe to retain after entity removal.
struct ProjectileImpactRecord {
    uint64_t projectileId = 0;
    uint64_t ownerId = 0;
    int team = 0;
    uint64_t targetId = 0;
    float time = 0.0f;
    Vector2 point;
    float requestedDamage = 0.0f;
    float appliedDamage = 0.0f; // HP actually removed, excluding overkill/armor
};

// Call-scoped target reference retained for existing direct consumers.
struct ProjectileImpact {
    uint64_t projectileId = 0;
    uint64_t targetId = 0;
    IGameUnit* target = nullptr;
    float time = 0.0f;
    Vector2 point;
};

// Explicit motion over the SAME interval as the projectile step.
// Hub uses the tick's start and post-contact end positions (linear chord).
struct ProjectileTargetMotion {
    IGameUnit* unit = nullptr;
    Vector2 start;
    Vector2 end;
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

    /// Positions are interval starts; target ends use constant velocity. Applies `IUnitCombat::TakeDamage` when the projectile damage is positive.
    std::vector<ProjectileImpact> Advance(float deltaTime, const std::vector<IGameUnit*>& units);

    std::vector<ProjectileImpact> AdvanceSwept(float deltaTime, const std::vector<ProjectileTargetMotion>& targets);
    const std::vector<ProjectileImpactRecord>& LastImpacts() const { return lastImpacts_; }

    const std::vector<Projectile>& Projectiles() const { return projectiles_; }
    uint64_t NextId() const { return nextId_; }

    void Restore(std::vector<Projectile> projectiles, uint64_t nextId);

    /// Constant-speed intercept of a target moving at `targetVelocity`. Independent of firing policy.
    static InterceptSolution Intercept(Vector2 origin, float speed, Vector2 targetPosition, Vector2 targetVelocity);

private:
    std::vector<Projectile> projectiles_;
    uint64_t nextId_ = 1;
    std::vector<ProjectileImpactRecord> lastImpacts_;
};
