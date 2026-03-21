#include "StateMachine.h"
#include "BehaviorContext.h"
#include "../interfaces/IGameUnit.h"
#include "../logging/Logger.h"

void StateMachine::AddState(const std::string& name, std::unique_ptr<IBehavior> behavior)
{
    if (!behavior) {
        LOG_ERROR("StateMachine::AddState: null behavior for state '%s'", name.c_str());
        return;
    }

    states_[name] = std::move(behavior);

    if (currentState_.empty()) {
        currentState_ = name;
        if (owner_ && states_[name]) {
            states_[name]->OnEnter(owner_);
        }
    }
}

void StateMachine::AddTransition(StateTransition transition)
{
    transitions_.push_back(std::move(transition));
}

Vector2 StateMachine::Update(const BehaviorContext& ctx)
{
    for (const auto& transition : transitions_) {
        if (!transition.isAutomatic) continue;
        if (transition.fromState != currentState_) continue;
        if (transition.condition && transition.condition->Evaluate(ctx)) {
            SetState(transition.toState);
            break;
        }
    }

    auto it = states_.find(currentState_);
    if (it != states_.end() && it->second) {
        return it->second->CalculateAcceleration(ctx);
    }

    return Vector2::Zero();
}

void StateMachine::SetState(const std::string& stateName)
{
    if (stateName == currentState_) return;

    auto it = states_.find(stateName);
    if (it == states_.end()) {
        LOG_ERROR("StateMachine::SetState: unknown state '%s'", stateName.c_str());
        return;
    }

    if (!currentState_.empty()) {
        auto cur = states_.find(currentState_);
        if (cur != states_.end() && cur->second && owner_) {
            cur->second->OnExit(owner_);
        }
    }

    currentState_ = stateName;

    if (it->second && owner_) {
        it->second->OnEnter(owner_);
    }
}

IBehavior* StateMachine::GetCurrentBehavior()
{
    auto it = states_.find(currentState_);
    return (it != states_.end()) ? it->second.get() : nullptr;
}

const IBehavior* StateMachine::GetCurrentBehavior() const
{
    auto it = states_.find(currentState_);
    return (it != states_.end()) ? it->second.get() : nullptr;
}
