#include <gtest/gtest.h>

#include "core/ai_advanced/GeneticTrainer.h"
#include "core/ai_advanced/MonteCarloRunner.h"

#include <cstdint>

namespace {

TEST(MonteCarloRunnerTest, RunMatrixAggregatesWinRatesDrawsAndTurns)
{
    const int numArchetypes = 2;
    const int matchesPerPair = 3;
    const uint64_t baseSeed = 42;

    int callCount = 0;
    const auto simFn = [&](int archetypeA, int archetypeB, uint64_t /*seed*/, int& outTurns) {
        ++callCount;
        outTurns = 10 + archetypeA + archetypeB;
        if (archetypeA == archetypeB)
            return 0;
        return archetypeA < archetypeB ? 1 : -1;
    };

    const auto matrix =
        CoreEngine::MonteCarloRunner::RunMatrix(numArchetypes, matchesPerPair, baseSeed, simFn);

    ASSERT_EQ(matrix.size(), static_cast<size_t>(numArchetypes));
    ASSERT_EQ(matrix[0].size(), static_cast<size_t>(numArchetypes));
    EXPECT_EQ(callCount, numArchetypes * numArchetypes * matchesPerPair);

    const CoreEngine::MatchupCellStats& offDiag01 = matrix[0][1];
    EXPECT_EQ(offDiag01.winsA, matchesPerPair);
    EXPECT_EQ(offDiag01.winsB, 0);
    EXPECT_EQ(offDiag01.draws, 0);
    EXPECT_FLOAT_EQ(offDiag01.WinRateA(), 1.0f);
    EXPECT_FLOAT_EQ(offDiag01.AverageTurns(), 11.0f);

    const CoreEngine::MatchupCellStats& offDiag10 = matrix[1][0];
    EXPECT_EQ(offDiag10.winsA, 0);
    EXPECT_EQ(offDiag10.winsB, matchesPerPair);
    EXPECT_FLOAT_EQ(offDiag10.WinRateA(), 0.0f);

    const CoreEngine::MatchupCellStats& diag = matrix[0][0];
    EXPECT_EQ(diag.draws, matchesPerPair);
    EXPECT_FLOAT_EQ(diag.WinRateA(), 0.0f);
    EXPECT_FLOAT_EQ(diag.AverageTurns(), 10.0f);
}

TEST(MonteCarloRunnerTest, GeneticTrainerCompilesWithoutBotPlayer)
{
    CoreEngine::GeneticTrainer trainer;
    trainer.Init();
    EXPECT_EQ(trainer.GetPopulation().size(), 20u);
    EXPECT_EQ(trainer.GetGenerationNumber(), 0);
}

} // namespace