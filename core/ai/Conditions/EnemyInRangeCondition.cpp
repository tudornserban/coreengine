#include "EnemyInRangeCondition.h"
#include "core/ai/BehaviorContext.h"
#include "core/interfaces/IGameUnit.h"
#include <cstdio>

EnemyInRangeCondition::EnemyInRangeCondition(float detectionRange) : range_(detectionRange) {}

bool EnemyInRangeCondition::Evaluate(const BehaviorContext& ctx) const
{
    if (!ctx.self) return false;
    Vector2 pos = ctx.self->GetPosition();
    for (IGameUnit* enemy : ctx.nearbyEnemies) {
        if (enemy->GetPosition().Distance(pos) <= range_) return true;
    }
    return false;
}

const char* EnemyInRangeCondition::GetDescription() const
{
    snprintf(description_, sizeof(description_), "Enemy within %.0f units", range_);
    return description_;
}
