#pragma once

#include <string>
#include <map>
#include <utility>
#include <vector>
#include <memory>
#include "core/interfaces/IBehavior.h"
#include "core/interfaces/ITransitionCondition.h"
#include "core/math/Vector2.h"

// Forward declaration
struct BehaviorContext;
class IGameUnit;

struct StateTransition {
    std::string fromState;
    std::string toState;
    std::unique_ptr<ITransitionCondition> condition;
    bool isAutomatic;

    StateTransition(const std::string& from, const std::string& to,
                    std::unique_ptr<ITransitionCondition> cond, bool automatic = true)
        : fromState(from), toState(to), condition(std::move(cond)), isAutomatic(automatic) {}
};

/// Generic finite state machine driven by IBehavior states and ITransitionCondition edges.
/// Used by every game for AI-driven unit behavior.
/// RationalInsanity uses SetState() at runtime to implement the "magic dust swap" mechanic.
class StateMachine {
private:
    std::string currentState_;
    std::map<std::string, std::unique_ptr<IBehavior>> states_;
    std::vector<StateTransition> transitions_;
    IGameUnit* owner_;

public:
    explicit StateMachine(IGameUnit* owner = nullptr) : owner_(owner) {}

    void SetOwner(IGameUnit* newOwner) { owner_ = newOwner; }

    void AddState(const std::string& name, std::unique_ptr<IBehavior> behavior);
    void AddTransition(StateTransition transition);

    /// Result of one automatic-transition pass. Priority is registration order:
    /// the first automatic transition whose condition is true wins. Manual
    /// (`isAutomatic == false`) transitions are ignored. Enter/exit run once.
    struct TransitionEval {
        bool changed = false;
        std::string fromState;
        std::string toState;
    };

    /// Transition pass only. Does not evaluate steering or advance behavior timers
    /// beyond what the winning condition itself reads.
    TransitionEval EvaluateAutomaticTransitions(const BehaviorContext& ctx);

    /// Evaluates automatic transitions, then returns the current behavior's acceleration.
    Vector2 Update(const BehaviorContext& ctx);

    /// Manually override current state (used by RationalInsanity magic dust swap, commands, etc.)
    void SetState(const std::string& stateName);

    /// Exit the current behavior and clear the active state (no current behavior until SetState/AddState).
    /// Used when ejecting Magic Dust so the unit receives no AI acceleration.
    void ClearCurrentState();

    const std::string& GetCurrentState() const { return currentState_; }
    const std::map<std::string, std::unique_ptr<IBehavior>>& GetStates() const { return states_; }
    std::map<std::string, std::unique_ptr<IBehavior>>& GetStates() { return states_; }

    IBehavior* GetCurrentBehavior();
    const IBehavior* GetCurrentBehavior() const;

    std::size_t TransitionCount() const { return transitions_.size(); }
    bool TransitionHasNumericState(std::size_t index) const;
    float TransitionNumericState(std::size_t index) const;
    void SetTransitionNumericState(std::size_t index, float value);

    /// Assign the current state without enter/exit callbacks. Fails if `name` is unknown.
    /// An empty name clears the current state without callbacks.
    bool RestoreCurrentStateName(const std::string& name);

    /// Replace one behavior's parameter map. Keys are inserted; unknown names fail.
    bool RestoreBehaviorParameters(const std::string& stateName,
                                   const std::vector<std::pair<std::string, float>>& parameters);
};
