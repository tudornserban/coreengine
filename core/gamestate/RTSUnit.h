#pragma once

#include "../interfaces/IGameUnit.h"
#include "../interfaces/IUnitKinematics.h"
#include "../interfaces/IUnitAgent.h"
#include "../interfaces/IUnitBehaviorHost.h"
#include "../ai/StateMachine.h"
#include "../math/Vector2.h"
#include <algorithm>
#include <string>

class RTSUnit : public IGameUnit, public IUnitKinematics, public IUnitAgent, public IUnitBehaviorHost {
protected:
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;

    float maxSpeed;
    float radius;
    int team;
    bool selected;

    // HP and Mass
    float currentHP;
    float maxHP;
    float baseMass;
    float massModifier;

    // Collision properties
    float restitution;
    float positionCorrection;
    float damageModifier;

    StateMachine stateMachine;

    // Debug mode
    bool debugMode;
    Vector2 debugDestination;

public:
    RTSUnit(Vector2 startPos, int teamId, float unitRadius = 10.0f, float unitMaxSpeed = 100.0f);
    virtual ~RTSUnit() = default;

    // IGameUnit implementation
    void Update(float deltaTime) override;
    Vector2 GetPosition() const override { return position; }
    Vector2 GetVelocity() const override { return velocity; }
    void ApplyAcceleration(Vector2 accel, float deltaTime) override;

    bool IsSelected() const override { return selected; }
    void SetSelected(bool sel) override { selected = sel; }

    int GetTeam() const override { return team; }
    float GetRadius() const override { return radius; }
    float GetVisualRadius() const {
        if (maxHP <= 0.0f) return radius;
        float hpRatio = currentHP / maxHP;
        return radius * hpRatio;
    }
    float GetMaxSpeed() const override { return maxSpeed; }

    void SetDebugDestination(Vector2 target) override {
        debugDestination = target;
        debugMode = true;
    }
    bool IsInDebugMode() const override { return debugMode; }
    void ClearDebugMode() { debugMode = false; }

    // State machine access
    StateMachine& GetStateMachine() { return stateMachine; }
    const StateMachine& GetStateMachine() const { return stateMachine; }

    // Setters (IUnitKinematics)
    void SetPosition(Vector2 pos) override { position = pos; }
    void SetVelocity(Vector2 vel) override { velocity = vel; }
    void SetMaxSpeed(float speed) { maxSpeed = speed; }

    // IUnitAgent (command surface for FSM / behavior parameters)
    void SetAgentState(const std::string& stateName) override;
    void SetAgentParameter(const std::string& paramName, float value) override;

    // IUnitBehaviorHost
    IBehavior* GetCurrentBehavior() override;

    // HP and Mass
    float GetCurrentHP() const override { return currentHP; }
    float GetMaxHP() const override { return maxHP; }
    float GetBaseMass() const override { return baseMass; }
    float GetMassModifier() const override { return massModifier; }
    float CalculateMass() const override;
    void SetHP(float hp) { currentHP = std::max(0.0f, std::min(maxHP, hp)); }
    void TakeDamage(float damage) { currentHP = std::max(0.0f, currentHP - damage); }

    // Collision properties
    float GetRestitution() const override { return restitution; }
    float GetPositionCorrection() const override { return positionCorrection; }
    float GetDamageModifier() const override { return damageModifier; }
    void SetRestitution(float rest) { restitution = rest; }
    void SetMassModifier(float mod) { massModifier = mod; }
    void SetDamageModifier(float mod) { damageModifier = mod; }
    void SetPositionCorrection(float corr) { positionCorrection = corr; }
};
