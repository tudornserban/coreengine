#include "ClusterDetector.h"

#include "core/math/ArenaGeometry.h"

#include <vector>

std::vector<std::vector<const IGameUnit*>> ClusterDetector::Detect(
    const std::vector<const IGameUnit*>& units,
    float clusterLinkDistance,
    bool wrapBorders,
    float arenaW,
    float arenaH)
{
    const float linkDistSq = clusterLinkDistance * clusterLinkDistance;
    const std::size_t n = units.size();
    std::vector<bool> visited(n, false);
    std::vector<std::vector<const IGameUnit*>> clusters;

    for (std::size_t i = 0; i < n; ++i) {
        if (visited[i] || !units[i]) {
            visited[i] = true;
            continue;
        }

        std::vector<const IGameUnit*> cluster;
        std::vector<std::size_t> stack;
        stack.push_back(i);
        visited[i] = true;

        while (!stack.empty()) {
            const std::size_t cur = stack.back();
            stack.pop_back();
            cluster.push_back(units[cur]);

            const Vector2 a = units[cur]->GetPosition();
            const int team = units[cur]->GetTeam();
            for (std::size_t j = 0; j < n; ++j) {
                if (visited[j] || !units[j] || units[j]->GetTeam() != team) {
                    continue;
                }
                const Vector2 b = units[j]->GetPosition();
                const float distSq = ArenaGeometry::DistanceSq(
                    a.x, a.y, b.x, b.y, arenaW, arenaH, wrapBorders);
                if (distSq <= linkDistSq) {
                    visited[j] = true;
                    stack.push_back(j);
                }
            }
        }

        clusters.push_back(std::move(cluster));
    }

    return clusters;
}
