#include "ResourceClaims.h"

#include "core/map/MapComponents/MapResourceComponent.h"

#include <algorithm>
#include <map>
#include <utility>

std::vector<ResourceGrant> ResolveResourceClaims(Map& map, const std::vector<ResourceClaim>& claims) {
    std::map<std::string, std::vector<uint64_t>> byResource;
    for (const ResourceClaim& claim : claims) {
        auto& claimants = byResource[claim.resourceEntityId];
        if (std::find(claimants.begin(), claimants.end(), claim.claimantId) == claimants.end()) {
            claimants.push_back(claim.claimantId);
        }
    }

    std::vector<ResourceGrant> grants;
    for (auto& entry : byResource) {
        std::vector<uint64_t>& claimants = entry.second;
        std::sort(claimants.begin(), claimants.end());
        MapEntity* entity = map.GetEntity(entry.first);
        if (!entity) {
            continue;
        }
        auto* resource = entity->GetComponent<MapResourceComponent>();
        if (!resource || resource->IsDepleted() || !(resource->GetAmount() > 0.0f)) {
            continue;
        }
        ResourceGrant grant;
        grant.claimantId = claimants.front();
        grant.resourceEntityId = entry.first;
        grant.amount = resource->GetAmount();
        resource->SetAmount(0.0f);
        grants.push_back(grant);
    }
    return grants;
}
