#include "StateMachine.h"
#include "BehaviorContext.h"
#include "core/interfaces/IGameUnit.h"
#include "core/logging/Logger.h"

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

StateMachine::TransitionEval StateMachine::EvaluateAutomaticTransitions(const BehaviorContext& ctx)
{
    TransitionEval result;
    result.fromState = currentState_;
    result.toState = currentState_;
    for (const auto& transition : transitions_) {
        if (!transition.isAutomatic) continue;
        if (transition.fromState != currentState_) continue;
        if (transition.condition && transition.condition->Evaluate(ctx)) {
            const std::string destination = transition.toState;
            SetState(destination);
            result.changed = currentState_ == destination && destination != result.fromState;
            result.toState = currentState_;
            break;
        }
    }
    return result;
}

Vector2 StateMachine::Update(const BehaviorContext& ctx)
{
    EvaluateAutomaticTransitions(ctx);

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

void StateMachine::ClearCurrentState()
{
    if (currentState_.empty()) {
        return;
    }
    auto cur = states_.find(currentState_);
    if (cur != states_.end() && cur->second && owner_) {
        cur->second->OnExit(owner_);
    }
    currentState_.clear();
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

bool StateMachine::TransitionHasNumericState(std::size_t index) const
{
    if (index >= transitions_.size() || !transitions_[index].condition) {
        return false;
    }
    return transitions_[index].condition->HasNumericState();
}

float StateMachine::TransitionNumericState(std::size_t index) const
{
    if (!TransitionHasNumericState(index)) {
        return 0.0f;
    }
    return transitions_[index].condition->GetNumericState();
}

void StateMachine::SetTransitionNumericState(std::size_t index, float value)
{
    if (!TransitionHasNumericState(index)) {
        return;
    }
    transitions_[index].condition->SetNumericState(value);
}

bool StateMachine::RestoreCurrentStateName(const std::string& name)
{
    if (name.empty()) {
        currentState_.clear();
        return true;
    }
    if (states_.find(name) == states_.end()) {
        return false;
    }
    currentState_ = name;
    return true;
}

bool StateMachine::RestoreBehaviorParameters(
    const std::string& stateName,
    const std::vector<std::pair<std::string, float>>& parameters)
{
    auto it = states_.find(stateName);
    if (it == states_.end() || !it->second) {
        return false;
    }
    auto& live = it->second->GetParameters();
    for (const auto& parameter : parameters) {
        auto found = live.find(parameter.first);
        if (found == live.end()) {
            return false;
        }
    }
    for (const auto& parameter : parameters) {
        live[parameter.first] = parameter.second;
    }
    return true;
}
