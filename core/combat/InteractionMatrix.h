#pragma once

#include <cstdint>
#include <vector>

namespace CoreEngine {

enum class ClashDominance : int8_t { DefenderWins = -1, Neutral = 0, AttackerWins = 1 };

/// Generic N x N matrix for archetype multipliers (e.g., 5-Precept Wheel, 4-Element Table)
/// and discrete RPS dominance (e.g., 5-Move Pentagon: Attack/Defend/Feint/Counter/Exploit).
class InteractionMatrix {
public:
    explicit InteractionMatrix(int dimension = 5, float defaultMultiplier = 1.0f);

    void SetMultiplier(int attackerIdx, int defenderIdx, float multiplier);
    float GetMultiplier(int attackerIdx, int defenderIdx) const;

    void SetDominance(int moveA, int moveB, ClashDominance outcome);
    ClashDominance GetDominance(int moveA, int moveB) const;

    int Dimension() const { return dimension_; }

    /// Helper factory for symmetric N-element RPS rings/wheels.
    static InteractionMatrix CreateFiveWayPentagon();
    static InteractionMatrix CreateFivePreceptWheel(float winMult = 1.3f, float loseMult = 0.7f);

private:
    int index(int row, int col) const;
    bool inBounds(int row, int col) const;

    int dimension_;
    float defaultMultiplier_;
    std::vector<float> multipliers_;
    std::vector<int8_t> dominance_;
};

}  // namespace CoreEngine