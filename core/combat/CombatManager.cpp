#include "CombatManager.h"

#include "core/interfaces/IInfluenceEmitter.h"
#include "core/interfaces/IUnitCombat.h"
#include "core/math/ArenaGeometry.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

IUnitCombat* CombatOf(IGameUnit* unit) {
    return dynamic_cast<IUnitCombat*>(unit);
}

const IUnitCombat* CombatOf(const IGameUnit* unit) {
    return dynamic_cast<const IUnitCombat*>(unit);
}

const IInfluenceEmitter* EmitterOf(const IGameUnit* unit) {
    return dynamic_cast<const IInfluenceEmitter*>(unit);
}

bool IsAlive(const IGameUnit* unit) {
    const IUnitCombat* combat = CombatOf(unit);
    return unit && combat && !combat->IsDead();
}

bool AreHostile(const IGameUnit* a, const IGameUnit* b, const CombatManager::Params& params) {
    if (!a || !b || a == b) {
        return false;
    }
    if (a->GetTeam() == b->GetTeam()) {
        return false;
    }
    if (params.areAllies && params.areAllies(a, b)) {
        return false;
    }
    return true;
}

float Multiplier(const IGameUnit* attacker, const IGameUnit* defender,
                 const CombatManager::Params& params) {
    float scale = params.damageMultiplier ? params.damageMultiplier(attacker, defender) : 1.0f;
    if (scale < 0.0f) {
        scale = 0.0f;
    }
    return scale;
}

float PairDistance(const IGameUnit* a, const IGameUnit* b, const CombatManager::Params& params) {
    const Vector2 pa = a->GetPosition();
    const Vector2 pb = b->GetPosition();
    return ArenaGeometry::Distance(pa.x, pa.y, pb.x, pb.y, params.arenaW, params.arenaH, params.wrapBorders);
}

const IGameUnit* SelectTarget(const IGameUnit* attacker, const IUnitCombat* combat,
                              const std::vector<IGameUnit*>& units,
                              const CombatManager::Params& params) {
    const IGameUnit* best = nullptr;
    float bestDist = 0.0f;
    float bestHp = 0.0f;
    float bestThreat = 0.0f;
    const float range = combat->GetAttackRange();
    const float rangeSq = range * range;

    for (IGameUnit* candidate : units) {
        if (!IsAlive(candidate) || !AreHostile(attacker, candidate, params)) {
            continue;
        }
        if (params.targetFilter && !params.targetFilter(attacker, candidate)) {
            continue;
        }
        const Vector2 pa = attacker->GetPosition();
        const Vector2 pb = candidate->GetPosition();
        const float distSq = ArenaGeometry::DistanceSq(
            pa.x, pa.y, pb.x, pb.y, params.arenaW, params.arenaH, params.wrapBorders);
        if (distSq > rangeSq) {
            continue;
        }
        const float dist = std::sqrt(distSq);
        const float hp = candidate->GetCurrentHP();
        float threat = 0.0f;
        if (const IUnitCombat* candidateCombat = CombatOf(candidate)) {
            threat = candidateCombat->GetBaseDPS();
        }

        bool better = false;
        if (!best) {
            better = true;
        } else if (params.targetPolicy == TargetPolicy::Nearest) {
            better = dist < bestDist || (dist == bestDist && candidate < best);
        } else if (params.targetPolicy == TargetPolicy::LowestHP) {
            better = hp < bestHp || (hp == bestHp && dist < bestDist);
        } else {
            better = threat > bestThreat || (threat == bestThreat && dist < bestDist);
        }
        if (better) {
            best = candidate;
            bestDist = dist;
            bestHp = hp;
            bestThreat = threat;
        }
    }
    return best;
}

}  // namespace

void CombatManager::ProcessAttrition(IGameUnit* unit, bool isSurrounded, float deltaTime,
                                     float surroundedHpFractionPerSecond) {
    IUnitCombat* combat = CombatOf(unit);
    if (combat && !combat->IsDead() && isSurrounded && deltaTime > 0.0f) {
        combat->TakeDamage(unit->GetMaxHP() * surroundedHpFractionPerSecond * deltaTime);
    }
}

void CombatManager::Resolve(const std::vector<IGameUnit*>& units, float deltaTime, const Params& params) {
    if (deltaTime <= 0.0f || units.empty()) {
        return;
    }

    std::vector<float> damage(units.size(), 0.0f);
    std::vector<int> zoneAttackers(units.size(), 0);
    std::unordered_map<const IGameUnit*, std::size_t> indexOf;
    indexOf.reserve(units.size());
    for (std::size_t i = 0; i < units.size(); ++i) {
        if (units[i]) {
            indexOf[units[i]] = i;
        }
    }

    const bool doZone = params.mode == CombatMode::InfluenceAttrition || params.mode == CombatMode::Hybrid;
    const bool doDirect = params.mode == CombatMode::DirectAttack || params.mode == CombatMode::Hybrid;

    if (doZone && params.zoneDamagePerSecond > 0.0f) {
        for (std::size_t i = 0; i < units.size(); ++i) {
            IGameUnit* a = units[i];
            if (!IsAlive(a) || !EmitterOf(a)) {
                continue;
            }
            for (std::size_t j = i + 1; j < units.size(); ++j) {
                IGameUnit* b = units[j];
                if (!IsAlive(b) || !EmitterOf(b) || !AreHostile(a, b, params)) {
                    continue;
                }
                const float reach = EmitterOf(a)->GetInfluenceRadius() + EmitterOf(b)->GetInfluenceRadius();
                const float dist = PairDistance(a, b, params);
                if (!(dist < reach)) {
                    continue;
                }
                const float closeness = reach > 0.0f ? (1.0f - dist / reach) : 1.0f;
                const float proximity = 1.0f + params.proximityDamageMultiplier * closeness * closeness;
                const float bodyReach = a->GetRadius() + b->GetRadius();
                const float collision = dist < bodyReach ? params.collisionDamageMultiplier : 1.0f;
                const float pairDamage = params.zoneDamagePerSecond * deltaTime * proximity * collision;
                damage[i] += pairDamage * Multiplier(b, a, params);
                damage[j] += pairDamage * Multiplier(a, b, params);
                ++zoneAttackers[i];
                ++zoneAttackers[j];
            }
        }

        for (std::size_t i = 0; i < units.size(); ++i) {
            if (zoneAttackers[i] >= 2) {
                const float modifier = 1.0f + params.outnumberedBonusPerExtraAttacker
                    * static_cast<float>(zoneAttackers[i] - 1);
                damage[i] *= modifier;
            }
        }
    }

    if (doDirect) {
        for (std::size_t i = 0; i < units.size(); ++i) {
            IGameUnit* attacker = units[i];
            IUnitCombat* combat = CombatOf(attacker);
            if (!IsAlive(attacker) || !combat || combat->GetBaseDPS() <= 0.0f) {
                continue;
            }
            const IGameUnit* target = SelectTarget(attacker, combat, units, params);
            if (!target) {
                continue;
            }

            const float cooldown = combat->GetAttackCooldown();
            int shots = 0;
            float shotDamage = 0.0f;
            if (cooldown <= 0.0f) {
                shots = 1;
                shotDamage = combat->GetBaseDPS() * deltaTime;
            } else {
                float& timer = TimerFor(attacker);
                timer += deltaTime;
                while (timer >= cooldown) {
                    timer -= cooldown;
                    ++shots;
                }
                shotDamage = combat->GetBaseDPS() * cooldown;
            }
            if (shots <= 0 || shotDamage <= 0.0f) {
                continue;
            }

            auto applyHit = [&](const IGameUnit* victim, float amount) {
                const auto found = indexOf.find(victim);
                if (found == indexOf.end() || amount <= 0.0f) {
                    return;
                }
                damage[found->second] += amount * static_cast<float>(shots) * Multiplier(attacker, victim, params);
            };

            applyHit(target, shotDamage);

            if (params.splashRadius > 0.0f) {
                const Vector2 impact = target->GetPosition();
                for (IGameUnit* victim : units) {
                    if (victim == target || !IsAlive(victim) || !AreHostile(attacker, victim, params)) {
                        continue;
                    }
                    const Vector2 pos = victim->GetPosition();
                    const float dist = ArenaGeometry::Distance(
                        impact.x, impact.y, pos.x, pos.y, params.arenaW, params.arenaH, params.wrapBorders);
                    if (dist > params.splashRadius) {
                        continue;
                    }
                    float falloff = 1.0f;
                    if (params.splashFalloff > 0.0f && params.splashRadius > 0.0f) {
                        falloff = 1.0f - params.splashFalloff * (dist / params.splashRadius);
                        if (falloff < 0.0f) {
                            falloff = 0.0f;
                        }
                    }
                    applyHit(victim, shotDamage * falloff);
                }
            }
        }
    }

    for (std::size_t i = 0; i < units.size(); ++i) {
        IGameUnit* unit = units[i];
        if (!IsAlive(unit)) {
            continue;
        }
        if (params.isSurrounded && params.isSurrounded(unit)) {
            damage[i] += unit->GetMaxHP() * params.surroundedHpFractionPerSecond * deltaTime;
        }
    }

    for (std::size_t i = 0; i < units.size(); ++i) {
        if (damage[i] <= 0.0f) {
            continue;
        }
        if (IUnitCombat* combat = CombatOf(units[i])) {
            combat->TakeDamage(damage[i]);
        }
    }

    for (auto it = attackTimers_.begin(); it != attackTimers_.end();) {
        const IGameUnit* unit = it->first;
        const bool present = indexOf.find(unit) != indexOf.end();
        if (!present || !IsAlive(unit) || (unit && unit->GetEntityId() != 0)) {
            it = attackTimers_.erase(it);
        } else {
            ++it;
        }
    }

    std::unordered_set<uint64_t> liveIds;
    for (IGameUnit* unit : units) {
        if (unit && unit->GetEntityId() != 0 && IsAlive(unit)) {
            liveIds.insert(unit->GetEntityId());
        }
    }
    for (auto it = attackTimersById_.begin(); it != attackTimersById_.end();) {
        if (liveIds.find(it->first) == liveIds.end()) {
            it = attackTimersById_.erase(it);
        } else {
            ++it;
        }
    }
}

float& CombatManager::TimerFor(const IGameUnit* unit) {
    const uint64_t id = unit ? unit->GetEntityId() : 0;
    if (id != 0) {
        return attackTimersById_[id];
    }
    return attackTimers_[unit];
}

std::vector<CombatManager::CooldownRecord> CombatManager::ExportCooldowns() const {
    std::vector<CooldownRecord> records;
    records.reserve(attackTimersById_.size());
    for (const auto& entry : attackTimersById_) {
        records.push_back(CooldownRecord{entry.first, entry.second});
    }
    std::sort(records.begin(), records.end(), [](const CooldownRecord& a, const CooldownRecord& b) {
        return a.entityId < b.entityId;
    });
    return records;
}

void CombatManager::ImportCooldowns(const std::vector<CooldownRecord>& records) {
    attackTimersById_.clear();
    for (const CooldownRecord& record : records) {
        if (record.entityId != 0) {
            attackTimersById_[record.entityId] = record.remaining;
        }
    }
}

void CombatManager::ForgetEntity(uint64_t entityId) {
    if (entityId != 0) {
        attackTimersById_.erase(entityId);
    }
}
