#include <gtest/gtest.h>

#include "core/combat/InteractionMatrix.h"

using CoreEngine::ClashDominance;
using CoreEngine::InteractionMatrix;

TEST(InteractionMatrixTest, FiveWayPentagonIsAntisymmetric) {
    const InteractionMatrix pentagon = InteractionMatrix::CreateFiveWayPentagon();
    ASSERT_EQ(pentagon.Dimension(), 5);

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            const auto forward = pentagon.GetDominance(i, j);
            const auto reverse = pentagon.GetDominance(j, i);
            if (i == j) {
                EXPECT_EQ(forward, ClashDominance::Neutral);
                EXPECT_EQ(reverse, ClashDominance::Neutral);
            } else if (forward == ClashDominance::AttackerWins) {
                EXPECT_EQ(reverse, ClashDominance::DefenderWins);
            } else if (forward == ClashDominance::DefenderWins) {
                EXPECT_EQ(reverse, ClashDominance::AttackerWins);
            }
        }
    }

    EXPECT_EQ(pentagon.GetDominance(0, 1), ClashDominance::AttackerWins);
    EXPECT_EQ(pentagon.GetDominance(0, 3), ClashDominance::DefenderWins);
}

TEST(InteractionMatrixTest, FivePreceptWheelAppliesWinAndLoseMultipliers) {
    constexpr float kWin = 1.3f;
    constexpr float kLose = 0.7f;
    const InteractionMatrix wheel = InteractionMatrix::CreateFivePreceptWheel(kWin, kLose);

    EXPECT_FLOAT_EQ(wheel.GetMultiplier(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(wheel.GetMultiplier(0, 1), kWin);
    EXPECT_FLOAT_EQ(wheel.GetMultiplier(0, 3), kLose);
    EXPECT_FLOAT_EQ(wheel.GetMultiplier(2, 2), 1.0f);
}

TEST(InteractionMatrixTest, CustomMatrixHonorsDefaultMultiplier) {
    InteractionMatrix matrix(3, 2.5f);
    matrix.SetMultiplier(1, 2, 4.0f);
    EXPECT_FLOAT_EQ(matrix.GetMultiplier(1, 2), 4.0f);
    EXPECT_FLOAT_EQ(matrix.GetMultiplier(0, 0), 2.5f);
    matrix.SetDominance(0, 1, ClashDominance::AttackerWins);
    EXPECT_EQ(matrix.GetDominance(0, 1), ClashDominance::AttackerWins);
}