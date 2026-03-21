#pragma once
#include "../../interfaces/IBehavior.h"

class WanderBehavior : public IBehavior {
private:
    std::map<std::string, float> params_;
    Vector2 wanderTarget_;
    float   timeSinceLastChange_;

public:
    WanderBehavior();

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& ctx) override { return {}; }
    Vector2 CalculateAcceleration(const BehaviorContext& ctx) override;
    const char* GetName() const override { return "Wander"; }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

private:
    Vector2 GetRandomDirection(const BehaviorContext& ctx);
};
