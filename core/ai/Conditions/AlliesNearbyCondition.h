#pragma once
#include "core/interfaces/ITransitionCondition.h"

class AlliesNearbyCondition : public ITransitionCondition {
private:
    int   minAllies_;
    float range_;
    mutable char description_[128];

public:
    AlliesNearbyCondition(int minimumAllies, float detectionRange);

    bool Evaluate(const BehaviorContext& ctx) const override;
    const char* GetDescription() const override;
};
