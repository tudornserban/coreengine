#include "AlliesNearbyCondition.h"
#include "../../ai/BehaviorContext.h"
#include "../../interfaces/IGameUnit.h"
#include <cstdio>

AlliesNearbyCondition::AlliesNearbyCondition(int minimumAllies, float detectionRange)
    : minAllies_(minimumAllies), range_(detectionRange) {}

bool AlliesNearbyCondition::Evaluate(const BehaviorContext& ctx) const
{
    if (!ctx.self) return false;
    Vector2 pos = ctx.self->GetPosition();
    int count = 0;
    for (IGameUnit* ally : ctx.nearbyAllies) {
        if (ally == ctx.self) continue;
        if (ally->GetPosition().Distance(pos) <= range_) ++count;
    }
    return count >= minAllies_;
}

const char* AlliesNearbyCondition::GetDescription() const
{
    snprintf(description_, sizeof(description_), "At least %d allies within %.0f units", minAllies_, range_);
    return description_;
}
