#pragma once

#include "core/interfaces/IBehavior.h"

/// Frontline steering: push away from nearby allies, push harder at contact
/// range, keep a gap from nearby enemies, and drift toward the local enemy group.
class TacticalFrontlineBehavior : public IBehavior {
private:
    std::map<std::string, float> params_;

public:
    TacticalFrontlineBehavior();

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& ctx) override { return {}; }
    Vector2 CalculateAcceleration(const BehaviorContext& ctx) override;
    const char* GetName() const override { return "TacticalFrontline"; }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }
};
