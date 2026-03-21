#pragma once

#include "core/interfaces/IBehavior.h"
#include "core/ai/BehaviorContext.h"
#include "core/influence/InfluenceFieldItem.h"
#include <map>
#include <string>
#include <vector>

/// Configurable test double for IBehavior.
class MockBehavior : public IBehavior {
private:
    std::string name_;
    Vector2     accelerationToReturn_;
    std::vector<InfluenceFieldItem> fieldItemsToReturn_;
    bool        returnFieldItems_ = false;
    std::map<std::string, float> params_;
    int enterCount_ = 0;
    int exitCount_  = 0;

public:
    explicit MockBehavior(const std::string& name = "MockBehavior")
        : name_(name), accelerationToReturn_(Vector2::Zero())
    {
        params_["strength"] = 100.0f;
    }

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& /*ctx*/) override {
        return returnFieldItems_ ? fieldItemsToReturn_ : std::vector<InfluenceFieldItem>{};
    }

    Vector2 CalculateAcceleration(const BehaviorContext& /*ctx*/) override {
        return accelerationToReturn_;
    }

    const char* GetName() const override { return name_.c_str(); }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

    void OnEnter(IGameUnit* /*unit*/) override { ++enterCount_; }
    void OnExit(IGameUnit* /*unit*/)  override { ++exitCount_; }

    // Configurators
    void SetAccelerationToReturn(Vector2 a)                                     { accelerationToReturn_ = a; returnFieldItems_ = false; }
    void SetFieldItemsToReturn(const std::vector<InfluenceFieldItem>& items)    { fieldItemsToReturn_ = items; returnFieldItems_ = true; }
    void SetParameter(const std::string& k, float v)                            { params_[k] = v; }

    // Observers
    int GetEnterCount() const { return enterCount_; }
    int GetExitCount()  const { return exitCount_; }
};
