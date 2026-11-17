#pragma once

#include "core/interfaces/IGameUnit.h"
#include "core/map/Obstacle.h"

#include <vector>

/// Grid liberty check. A cluster is encircled when no occupied cell has a
/// 4-connected neighbour that is unblocked and reachable from the map border.
/// Enemy influence discs and `blocksInfluence` obstacles mark blocked cells.
/// `wrapBorders` uses toroidal distances and wrapping neighbours. Border cells
/// are still the flood seeds, so a closed ring stays sealed on a torus.
class EncirclementChecker {
public:
    static constexpr int kDefaultCols = 40;
    static constexpr int kDefaultRows = 23;

    struct Query {
        int cols = kDefaultCols;
        int rows = kDefaultRows;
        float clusterLinkDistance = 300.0f;
        bool wrapBorders = false;
        std::vector<Obstacle> obstacles;
    };

    [[nodiscard]] static bool IsEncircled(
        const std::vector<const IGameUnit*>& cluster,
        const std::vector<const IGameUnit*>& enemies,
        float arenaW,
        float arenaH,
        const Query& query);
};
