#pragma once

class IBehavior;

/// Optional contract for units whose hub AI tick reads the active behavior.
/// Only concrete RTS units implement this; mocks without a state machine do not.
class IUnitBehaviorHost {
public:
    virtual ~IUnitBehaviorHost() = default;

    virtual IBehavior* GetCurrentBehavior() = 0;
};
