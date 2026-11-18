#pragma once

#include "SessionTypes.h"

#include <memory>
#include <vector>

namespace Session
{

// Game rules for one match. DynamicGo, AscendantExpanse, and ZodiacWars each
// supply an implementation. The session layer never includes those games.
//
// Reset, then for every tick: Apply that tick's commands in ascending player
// id order, then Step. Missing commands are not applied (the default action).
// Snapshot/Restore must round-trip the whole sim state so online rollback can
// replay a tick. Restore returns false without changing state when the bytes
// are not a snapshot this implementation wrote.
class IDeterministicMatch
{
public:
    virtual ~IDeterministicMatch() = default;

    // Game-side check of a lobby setup payload. The default accepts any payload
    // the session layer already validated. Return false to refuse the match.
    // Adapters advance exactly one simulation tick inside each Step call.
    virtual bool Configure(const MatchSetup& setup)
    {
        (void)setup;
        return true;
    }

    virtual void Reset(uint64_t seed, const std::vector<uint32_t>& playerIds) = 0;
    virtual void Apply(const PlayerCommand& command) = 0;
    virtual void Step() = 0;
    virtual uint64_t StateHash() const = 0;
    virtual bool IsComplete() const = 0;
    virtual MatchOutcome Outcome() const = 0;
    virtual uint32_t MaxTicks() const = 0;
    virtual std::vector<uint8_t> Snapshot() const = 0;
    virtual bool Restore(const std::vector<uint8_t>& bytes) = 0;
};

class IMatchFactory
{
public:
    virtual ~IMatchFactory() = default;
    virtual std::unique_ptr<IDeterministicMatch> Create(GameTitle game) = 0;
};

}
