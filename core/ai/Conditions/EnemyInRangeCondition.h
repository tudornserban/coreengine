#pragma once
#include "../../interfaces/ITransitionCondition.h"

class EnemyInRangeCondition : public ITransitionCondition {
private:
    float range_;
    mutable char description_[128];

public:
    explicit EnemyInRangeCondition(float detectionRange);

    bool Evaluate(const BehaviorContext& ctx) const override;
    const char* GetDescription() const override;

    void SetRange(float r) { range_ = r; }
    float GetRange() const { return range_; }
};
