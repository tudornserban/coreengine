#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "../interfaces/IBehavior.h"
#include "../interfaces/ITransitionCondition.h"
#include "../math/Vector2.h"

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
};
