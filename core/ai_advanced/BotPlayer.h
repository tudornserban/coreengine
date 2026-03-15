#pragma once

#include "Genome.h"
#include <functional>
#include <vector>

// Forward declaration
class RTSGameState;
class IGameUnit;

namespace CoreEngine {

/// Placement scoring function.
/// Receives: (x, y, teamId, gameState, genome) → score.
/// The default implementation uses the 6-feature DynamicGO evaluator.
/// Games override this to provide their own feature set.
using PlacementEvaluator = std::function<float(
    float x, float y, int teamId,
    const RTSGameState& state,
    const Genome& genome
)>;

/// AI player that uses a Genome to evaluate and select unit placements.
///
/// Works generically with any RTSGameState — does not depend on AnchorUnit.
/// Game-specific behavior is injected via PlacementEvaluator.
class BotPlayer {
public:
    BotPlayer(int teamId, const Genome& genome,
              int candidateGrid = 32,
              PlacementEvaluator evaluator = nullptr);

    /// Attempt to spawn a unit for the bot's team.
    /// Returns true if a spawn was issued.
    bool TrySpawn(RTSGameState& state,
                  std::function<bool(int teamId, float x, float y)> spawnFn) const;

    void SetGenome(const Genome& g) { genome_ = g; }
    const Genome& GetGenome() const { return genome_; }
    int GetTeamId() const { return teamId_; }

private:
    int teamId_;
    Genome genome_;
    int candidateGrid_;
    PlacementEvaluator evaluator_;

    float DefaultEvaluate(float x, float y, int teamId, const RTSGameState& state) const;
};

} // namespace CoreEngine
