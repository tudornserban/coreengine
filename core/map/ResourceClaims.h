#pragma once

#include "core/map/Map.h"

#include <cstdint>
#include <string>
#include <vector>

struct ResourceClaim {
    uint64_t claimantId = 0;
    std::string resourceEntityId;
};

struct ResourceGrant {
    uint64_t claimantId = 0;
    std::string resourceEntityId;
    float amount = 0.0f;
};

/// Consume-once collection. Simultaneous claims on one resource are won by the
/// lowest claimant id. The resource amount becomes 0. Restoring that amount
/// allows a later claim; restoring the depleted amount does not pay twice.
std::vector<ResourceGrant> ResolveResourceClaims(Map& map, const std::vector<ResourceClaim>& claims);
