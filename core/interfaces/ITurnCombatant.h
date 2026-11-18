#pragma once

#include <cstdint>

namespace CoreEngine {

/// Minimal contract for a participant in discrete turn-based / clash combat.
class ITurnCombatant {
public:
    virtual ~ITurnCombatant() = default;

    virtual int GetCombatantId() const = 0;
    virtual int GetArchetypeId() const = 0; // e.g. Precept or Element index [0..N-1]
    virtual float GetHP() const = 0;
    virtual float GetMaxHP() const = 0;
    virtual float GetStamina() const = 0;
    virtual float GetMaxStamina() const = 0;
    virtual int GetInitiativeMeter() const = 0;
    virtual int GetMomentum() const = 0;

    virtual bool IsDefeated() const { return GetHP() <= 0.0f; }
};

} // namespace CoreEngine