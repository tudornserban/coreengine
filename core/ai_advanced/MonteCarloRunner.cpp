#include "MonteCarloRunner.h"

namespace CoreEngine {

std::vector<std::vector<MatchupCellStats>> MonteCarloRunner::RunMatrix(int numArchetypes, int matchesPerPair,
                                                                       uint64_t baseSeed, SimulateMatchFn simFn)
{
    std::vector<std::vector<MatchupCellStats>> matrix(static_cast<size_t>(numArchetypes),
                                                      std::vector<MatchupCellStats>(static_cast<size_t>(numArchetypes)));

    uint64_t seed = baseSeed;
    for (int a = 0; a < numArchetypes; ++a) {
        for (int b = 0; b < numArchetypes; ++b) {
            MatchupCellStats& cell = matrix[static_cast<size_t>(a)][static_cast<size_t>(b)];
            for (int m = 0; m < matchesPerPair; ++m) {
                int turns = 0;
                const int outcome = simFn(a, b, seed++, turns);
                cell.totalTurns += static_cast<float>(turns);
                if (outcome > 0)
                    ++cell.winsA;
                else if (outcome < 0)
                    ++cell.winsB;
                else
                    ++cell.draws;
            }
        }
    }
    return matrix;
}

} // namespace CoreEngine