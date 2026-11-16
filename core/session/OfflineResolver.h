#pragma once

#include "IDeterministicMatch.h"

#include <vector>

namespace Session
{

// Runs the match to completion on the server (or on a client that wants to
// check the server's hash). Command submission order does not change the
// result: commands are applied by tick, then by player id.
MatchOutcome ResolveOffline(
    IDeterministicMatch& sim,
    uint64_t seed,
    const std::vector<uint32_t>& playerIds,
    const std::vector<DecisionPlan>& plans);

}
