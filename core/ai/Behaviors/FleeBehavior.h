#pragma once
#include "../../interfaces/IBehavior.h"

class FleeBehavior : public IBehavior {
private:
    std::map<std::string, float> params_;

public:
    FleeBehavior();

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& ctx) override { return {}; }
    Vector2 CalculateAcceleration(const BehaviorContext& ctx) override;
    const char* GetName() const override { return "Flee"; }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }
};
