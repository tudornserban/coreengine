#include "TimeElapsedCondition.h"
#include "../../ai/BehaviorContext.h"
#include <cstdio>

TimeElapsedCondition::TimeElapsedCondition(float durationSeconds)
    : duration_(durationSeconds), elapsed_(0.0f) {}

bool TimeElapsedCondition::Evaluate(const BehaviorContext& ctx) const
{
    elapsed_ += ctx.deltaTime;
    if (elapsed_ >= duration_) {
        elapsed_ = 0.0f;
        return true;
    }
    return false;
}

const char* TimeElapsedCondition::GetDescription() const
{
    snprintf(description_, sizeof(description_), "After %.1f seconds", duration_);
    return description_;
}
