#pragma once

#include "core/interfaces/IGameUnit.h"
#include "core/math/Vector2.h"
#include <algorithm>
#include <cmath>

class MockGameUnit : public IGameUnit {
private:
    Vector2 position_;
    Vector2 velocity_;
    int     team_;
    float   radius_;
    float   maxSpeed_;
    bool    selected_;
    float   currentHP_;
    float   maxHP_;
    float   baseMass_;
    float   massModifier_;
    float   restitution_;
    float   positionCorrection_;
    float   damageModifier_;
    bool    debugMode_;
    Vector2 debugDestination_;

public:
    explicit MockGameUnit(Vector2 position  = Vector2::Zero(),
                          int     team      = 0,
                          float   radius    = 10.0f,
                          float   maxSpeed  = 100.0f)
        : position_(position), velocity_(Vector2::Zero()), team_(team)
        , radius_(radius), maxSpeed_(maxSpeed), selected_(false)
        , currentHP_(100.0f), maxHP_(100.0f)
        , baseMass_(100.0f), massModifier_(1.0f)
        , restitution_(0.5f), positionCorrection_(0.8f)
        , damageModifier_(1.0f), debugMode_(false)
        , debugDestination_(Vector2::Zero())
    {}

    void    Update(float dt) override             { position_ += velocity_ * dt; }
    Vector2 GetPosition()    const override       { return position_; }
    Vector2 GetVelocity()    const override       { return velocity_; }

    void ApplyAcceleration(Vector2 accel, float dt) override {
        velocity_ += accel * dt;
        float speed = velocity_.Length();
        if (speed > maxSpeed_)
            velocity_ = velocity_.Normalized() * maxSpeed_;
    }

    bool  IsSelected()          const override { return selected_; }
    void  SetSelected(bool s)         override { selected_ = s; }
    int   GetTeam()             const override { return team_; }
    float GetRadius()           const override { return radius_; }
    float GetMaxSpeed()         const override { return maxSpeed_; }
    float GetCurrentHP()        const override { return currentHP_; }
    float GetMaxHP()            const override { return maxHP_; }
    float GetBaseMass()         const override { return baseMass_; }
    float GetMassModifier()     const override { return massModifier_; }
    float CalculateMass()       const override { return baseMass_ * (currentHP_ / maxHP_) * massModifier_; }
    float GetRestitution()      const override { return restitution_; }
    float GetPositionCorrection() const override { return positionCorrection_; }
    float GetDamageModifier()   const override { return damageModifier_; }

    void SetDebugDestination(Vector2 t) override { debugDestination_ = t; debugMode_ = true; }
    bool IsInDebugMode()        const override { return debugMode_; }

    // Test-only helpers
    void  SetPosition(Vector2 p) { position_ = p; }
    void  SetVelocity(Vector2 v) { velocity_ = v; }
    void  SetHP(float hp)        { currentHP_ = std::max(0.0f, std::min(maxHP_, hp)); }
    void  SetMaxHP(float hp)     { maxHP_ = hp; }
    void  TakeDamage(float dmg)  { currentHP_ = std::max(0.0f, currentHP_ - dmg); }
};
