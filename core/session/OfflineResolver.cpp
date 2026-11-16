#include "OfflineResolver.h"

#include <algorithm>
#include <map>

namespace Session
{

MatchOutcome ResolveOffline(
    IDeterministicMatch& sim,
    uint64_t seed,
    const std::vector<uint32_t>& playerIds,
    const std::vector<DecisionPlan>& plans)
{
    std::vector<uint32_t> ids = playerIds;
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    std::map<uint32_t, std::map<uint32_t, PlayerCommand>> byTick;
    for (const DecisionPlan& plan : plans)
    {
        if (std::find(ids.begin(), ids.end(), plan.playerId) == ids.end())
        {
            continue;
        }
        for (const PlayerCommand& command : plan.commands)
        {
            PlayerCommand copy = command;
            copy.playerId = plan.playerId;
            byTick[plan.playerId][command.tick] = copy;
        }
    }

    sim.Reset(seed, ids);
    uint32_t steps = 0;
    const uint32_t maxTicks = sim.MaxTicks();
    for (uint32_t tick = 0; tick < maxTicks && !sim.IsComplete(); ++tick)
    {
        for (uint32_t playerId : ids)
        {
            const auto player = byTick.find(playerId);
            if (player == byTick.end())
            {
                continue;
            }
            const auto atTick = player->second.find(tick);
            if (atTick == player->second.end())
            {
                continue;
            }
            sim.Apply(atTick->second);
        }
        sim.Step();
        ++steps;
    }

    MatchOutcome outcome = sim.Outcome();
    outcome.stateHash = sim.StateHash();
    outcome.ticksSimulated = steps;
    outcome.complete = sim.IsComplete() || (maxTicks > 0 && steps >= maxTicks);
    outcome.forfeit = false;
    return outcome;
}

}
