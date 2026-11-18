#include "ProjectileSimulation.h"

#include "core/interfaces/IUnitCombat.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

bool EarliestApproach(Vector2 origin, Vector2 velocity, Vector2 target, Vector2 targetVelocity,
                      float radiusSum, float deltaTime, float& time, Vector2& point) {
    const Vector2 relativePosition = origin - target;
    const Vector2 relativeVelocity = velocity - targetVelocity;
    const float radius = std::max(0.0f, radiusSum);
    const float c = relativePosition.LengthSquared() - radius * radius;
    if (c <= 0.0f) {
        time = 0.0f;
        point = origin;
        return true;
    }

    const float a = relativeVelocity.LengthSquared();
    if (!(a > 0.0f)) {
        return false;
    }
    const float b = 2.0f * relativePosition.Dot(relativeVelocity);
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    }
    const float root = std::sqrt(discriminant);
    float hitTime = (-b - root) / (2.0f * a);
    if (hitTime < 0.0f) {
        hitTime = (-b + root) / (2.0f * a);
    }
    if (hitTime < 0.0f || hitTime > deltaTime) {
        return false;
    }
    time = hitTime;
    point = origin + velocity * hitTime;
    return true;
}

}  // namespace

uint64_t ProjectileSimulation::Spawn(Projectile projectile) {
    if (projectile.id == 0) {
        projectile.id = nextId_++;
    } else {
        nextId_ = std::max(nextId_, projectile.id + 1);
    }
    const uint64_t id = projectile.id;
    projectiles_.push_back(projectile);
    return id;
}

std::vector<ProjectileImpact> ProjectileSimulation::Advance(float deltaTime, const std::vector<IGameUnit*>& units) {
    std::vector<ProjectileTargetMotion> targets;
    for (IGameUnit* unit : units) {
        if (unit) targets.push_back({unit, unit->GetPosition(),
            unit->GetPosition() + unit->GetVelocity() * deltaTime});
    }
    return AdvanceSwept(deltaTime, targets);
}

std::vector<ProjectileImpact> ProjectileSimulation::AdvanceSwept(
    float deltaTime, const std::vector<ProjectileTargetMotion>& targets) {
    lastImpacts_.clear();
    std::vector<ProjectileImpact> candidates;
    if (!(deltaTime > 0.0f)) {
        return candidates;
    }

    for (const Projectile& projectile : projectiles_) {
        if (projectile.timeRemaining <= 0.0f) {
            continue;
        }
        const float window = std::min(deltaTime, projectile.timeRemaining);
        for (const auto& motion : targets) {
            IGameUnit* unit = motion.unit;
            if (!unit || !unit->ParticipatesInCollision() || unit->GetCurrentHP() <= 0) {
                continue;
            }
            if (unit->GetEntityId() != 0 && unit->GetEntityId() == projectile.ownerId) {
                continue;
            }
            if (!projectile.hitAllies && unit->GetTeam() == projectile.team) {
                continue;
            }
            float time = 0.0f;
            Vector2 point;
            const float reach = projectile.radius + unit->GetRadius();
            if (!EarliestApproach(projectile.position, projectile.velocity, motion.start,
                                  (motion.end - motion.start) / deltaTime, reach, window, time, point)) {
                continue;
            }
            ProjectileImpact impact;
            impact.projectileId = projectile.id;
            impact.targetId = unit->GetEntityId();
            impact.target = unit;
            impact.time = time;
            impact.point = point;
            candidates.push_back(impact);
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const ProjectileImpact& a, const ProjectileImpact& b) {
        if (a.time != b.time) {
            return a.time < b.time;
        }
        if (a.projectileId != b.projectileId) {
            return a.projectileId < b.projectileId;
        }
        return a.targetId < b.targetId;
    });

    std::vector<ProjectileImpact> resolved;
    std::vector<uint64_t> consumed;
    for (const ProjectileImpact& impact : candidates) {
        if (std::find(consumed.begin(), consumed.end(), impact.projectileId) != consumed.end()) {
            continue;
        }
        if (!impact.target || !impact.target->ParticipatesInCollision() || impact.target->GetCurrentHP() <= 0)
            continue; // A prior impact may have killed this target; try the next candidate.
        consumed.push_back(impact.projectileId);
        resolved.push_back(impact);
        auto projectile = std::find_if(projectiles_.begin(), projectiles_.end(), [&](const Projectile& item) {
            return item.id == impact.projectileId;
        });
        if (projectile != projectiles_.end() && impact.target) {
            const float hpBefore = impact.target->GetCurrentHP();
            if (auto* combat = dynamic_cast<IUnitCombat*>(impact.target)) {
                if (projectile->damage > 0.0f) {
                    combat->TakeDamage(projectile->damage);
                }
            }
            lastImpacts_.push_back({impact.projectileId, projectile->ownerId, projectile->team,
                impact.targetId, impact.time, impact.point, std::max(0.0f, projectile->damage),
                std::max(0.0f, std::max(0.0f, hpBefore) - std::max(0.0f, impact.target->GetCurrentHP()))});
        }
    }

    std::vector<Projectile> survivors;
    survivors.reserve(projectiles_.size());
    for (Projectile& projectile : projectiles_) {
        const bool hit = std::find(consumed.begin(), consumed.end(), projectile.id) != consumed.end();
        if (hit) {
            continue;
        }
        projectile.timeRemaining -= deltaTime;
        if (projectile.timeRemaining <= 0.0f) {
            continue;
        }
        projectile.position += projectile.velocity * deltaTime;
        survivors.push_back(projectile);
    }
    projectiles_.swap(survivors);
    return resolved;
}

void ProjectileSimulation::Restore(std::vector<Projectile> projectiles, uint64_t nextId) {
    lastImpacts_.clear();
    projectiles_ = std::move(projectiles);
    nextId_ = std::max<uint64_t>(1, nextId);
}

InterceptSolution ProjectileSimulation::Intercept(Vector2 origin, float speed, Vector2 targetPosition,
                                                   Vector2 targetVelocity) {
    InterceptSolution solution;
    if (!(speed > 0.0f)) {
        return solution;
    }
    const Vector2 toTarget = targetPosition - origin;
    const float a = targetVelocity.LengthSquared() - speed * speed;
    const float b = 2.0f * toTarget.Dot(targetVelocity);
    const float c = toTarget.LengthSquared();
    float time = 0.0f;
    if (std::abs(a) < 1.0e-8f) {
        if (std::abs(b) < 1.0e-8f) {
            return solution;
        }
        time = -c / b;
    } else {
        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f) {
            return solution;
        }
        const float root = std::sqrt(discriminant);
        const float first = (-b - root) / (2.0f * a);
        const float second = (-b + root) / (2.0f * a);
        if (first >= 0.0f && second >= 0.0f) {
            time = std::min(first, second);
        } else if (first >= 0.0f) {
            time = first;
        } else {
            time = second;
        }
    }
    if (!(time >= 0.0f) || !std::isfinite(time)) {
        return solution;
    }
    solution.hit = true;
    solution.time = time;
    solution.point = targetPosition + targetVelocity * time;
    return solution;
}
