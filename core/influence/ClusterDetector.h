#pragma once

#include "core/interfaces/IGameUnit.h"

#include <vector>

/// Proximity connected components. Units link when they share a team and the
/// arena distance between them is within `clusterLinkDistance`.
class ClusterDetector {
public:
    /// DynamicGO default: 2 × a 150px influence radius.
    static constexpr float kDefaultLinkDistance = 300.0f;

    [[nodiscard]] static std::vector<std::vector<const IGameUnit*>> Detect(
        const std::vector<const IGameUnit*>& units,
        float clusterLinkDistance = kDefaultLinkDistance,
        bool wrapBorders = false,
        float arenaW = 0.0f,
        float arenaH = 0.0f);
};
