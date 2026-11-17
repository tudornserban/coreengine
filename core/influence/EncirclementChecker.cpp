#include "EncirclementChecker.h"

#include "core/interfaces/IInfluenceEmitter.h"
#include "core/math/ArenaGeometry.h"

#include <queue>
#include <vector>

bool EncirclementChecker::IsEncircled(
    const std::vector<const IGameUnit*>& cluster,
    const std::vector<const IGameUnit*>& enemies,
    float arenaW,
    float arenaH,
    const Query& query)
{
    if (cluster.empty() || query.cols < 1 || query.rows < 1) {
        return false;
    }
    if (!(arenaW > 0.0f) || !(arenaH > 0.0f)) {
        return false;
    }

    const int cols = query.cols;
    const int rows = query.rows;
    const float cellW = arenaW / static_cast<float>(cols);
    const float cellH = arenaH / static_cast<float>(rows);
    const int total = cols * rows;
    const float linkDistSq = query.clusterLinkDistance * query.clusterLinkDistance;
    const bool wrap = query.wrapBorders;

    std::vector<bool> blocked(static_cast<std::size_t>(total), false);
    std::vector<bool> occupied(static_cast<std::size_t>(total), false);
    std::vector<bool> reachable(static_cast<std::size_t>(total), false);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const float cx = (static_cast<float>(c) + 0.5f) * cellW;
            const float cy = (static_cast<float>(r) + 0.5f) * cellH;
            const int idx = r * cols + c;

            for (const IGameUnit* enemy : enemies) {
                if (!enemy) {
                    continue;
                }
                const auto* emitter = dynamic_cast<const IInfluenceEmitter*>(enemy);
                if (!emitter) {
                    continue;
                }
                const float er = emitter->GetInfluenceRadius();
                const Vector2 pos = enemy->GetPosition();
                const float distSq = ArenaGeometry::DistanceSq(
                    cx, cy, pos.x, pos.y, arenaW, arenaH, wrap);
                if (distSq <= er * er) {
                    blocked[static_cast<std::size_t>(idx)] = true;
                    break;
                }
            }

            if (!blocked[static_cast<std::size_t>(idx)]) {
                for (const Obstacle& obstacle : query.obstacles) {
                    if (!obstacle.blocksInfluence) {
                        continue;
                    }
                    const float distSq = ArenaGeometry::DistanceSq(
                        cx, cy, obstacle.x, obstacle.y, arenaW, arenaH, wrap);
                    if (distSq <= obstacle.radius * obstacle.radius) {
                        blocked[static_cast<std::size_t>(idx)] = true;
                        break;
                    }
                }
            }

            for (const IGameUnit* unit : cluster) {
                if (!unit) {
                    continue;
                }
                const Vector2 pos = unit->GetPosition();
                const float distSq = ArenaGeometry::DistanceSq(
                    cx, cy, pos.x, pos.y, arenaW, arenaH, wrap);
                if (distSq <= linkDistSq) {
                    occupied[static_cast<std::size_t>(idx)] = true;
                    break;
                }
            }
        }
    }

    std::queue<int> bfs;
    auto enqueue = [&](int c, int r) {
        const int i = r * cols + c;
        if (!blocked[static_cast<std::size_t>(i)] && !reachable[static_cast<std::size_t>(i)]) {
            reachable[static_cast<std::size_t>(i)] = true;
            bfs.push(i);
        }
    };

    for (int c = 0; c < cols; ++c) {
        enqueue(c, 0);
        enqueue(c, rows - 1);
    }
    for (int r = 1; r < rows - 1; ++r) {
        enqueue(0, r);
        enqueue(cols - 1, r);
    }

    constexpr int kDc[4] = {1, -1, 0, 0};
    constexpr int kDr[4] = {0, 0, 1, -1};

    while (!bfs.empty()) {
        const int cur = bfs.front();
        bfs.pop();
        const int c = cur % cols;
        const int r = cur / cols;
        for (int d = 0; d < 4; ++d) {
            int nc = c + kDc[d];
            int nr = r + kDr[d];
            if (wrap) {
                nc = (nc % cols + cols) % cols;
                nr = (nr % rows + rows) % rows;
                enqueue(nc, nr);
            } else if (nc >= 0 && nc < cols && nr >= 0 && nr < rows) {
                enqueue(nc, nr);
            }
        }
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (!occupied[static_cast<std::size_t>(r * cols + c)]) {
                continue;
            }
            for (int d = 0; d < 4; ++d) {
                int nc = c + kDc[d];
                int nr = r + kDr[d];
                if (wrap) {
                    nc = (nc % cols + cols) % cols;
                    nr = (nr % rows + rows) % rows;
                } else if (nc < 0 || nc >= cols || nr < 0 || nr >= rows) {
                    continue;
                }
                const int ni = nr * cols + nc;
                if (!blocked[static_cast<std::size_t>(ni)] && reachable[static_cast<std::size_t>(ni)]) {
                    return false;
                }
            }
        }
    }

    return true;
}
