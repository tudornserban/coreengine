#pragma once
#include "core/interfaces/IBehavior.h"

class SeekBehavior : public IBehavior {
private:
    std::map<std::string, float> params_;
    Vector2 targetPosition_;

public:
    explicit SeekBehavior(Vector2 target = Vector2::Zero());

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& ctx) override { return {}; }
    Vector2 CalculateAcceleration(const BehaviorContext& ctx) override;
    const char* GetName() const override { return "Seek"; }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

    void SetTarget(Vector2 target) { targetPosition_ = target; }
    Vector2 GetTarget() const { return targetPosition_; }
};
