#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace CoreEngine {

struct MatchupCellStats {
    int winsA = 0;
    int winsB = 0;
    int draws = 0;
    float totalTurns = 0.0f;

    int TotalMatches() const { return winsA + winsB + draws; }
    float WinRateA() const { return TotalMatches() > 0 ? float(winsA) / float(TotalMatches()) : 0.0f; }
    float AverageTurns() const { return TotalMatches() > 0 ? totalTurns / float(TotalMatches()) : 0.0f; }
};

class MonteCarloRunner {
public:
    /// Outcome callback for a single simulated match given (archetypeA, archetypeB, deterministicSeed).
    /// Returns: +1 if A wins, -1 if B wins, 0 if draw; writes turn/step count to outTurns.
    using SimulateMatchFn =
        std::function<int(int archetypeA, int archetypeB, uint64_t seed, int& outTurns)>;

    static std::vector<std::vector<MatchupCellStats>> RunMatrix(int numArchetypes, int matchesPerPair,
                                                                  uint64_t baseSeed, SimulateMatchFn simFn);
};

} // namespace CoreEngine