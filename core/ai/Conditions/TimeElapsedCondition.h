#pragma once
#include "core/interfaces/ITransitionCondition.h"

class TimeElapsedCondition : public ITransitionCondition {
private:
    float duration_;
    mutable float elapsed_;
    mutable char description_[128];

public:
    explicit TimeElapsedCondition(float durationSeconds);

    bool Evaluate(const BehaviorContext& ctx) const override;
    const char* GetDescription() const override;

    void Reset() { elapsed_ = 0.0f; }
};
