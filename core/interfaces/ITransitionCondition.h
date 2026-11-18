#pragma once

// Forward declaration
struct BehaviorContext;

/// Evaluates a boolean condition from the current BehaviorContext.
/// Used by StateMachine to drive automatic state transitions.
class ITransitionCondition {
public:
    virtual ~ITransitionCondition() = default;

    virtual bool Evaluate(const BehaviorContext& ctx) const = 0;

    /// Human-readable description for editors and debugging.
    virtual const char* GetDescription() const = 0;

    /// Optional scalar carried across snapshot restore (for example an elapsed timer).
    virtual bool HasNumericState() const { return false; }
    virtual float GetNumericState() const { return 0.0f; }
    virtual void SetNumericState(float /*value*/) {}
};
