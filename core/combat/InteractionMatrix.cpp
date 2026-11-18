#include "InteractionMatrix.h"

#include <cmath>

namespace CoreEngine {

namespace {

bool pentagonAttackerWins(int attacker, int defender, int dimension) {
    if (attacker == defender) {
        return false;
    }
    const int delta = (defender - attacker + dimension) % dimension;
    return delta == 1 || delta == 2;
}

}  // namespace

InteractionMatrix::InteractionMatrix(int dimension, float defaultMultiplier)
    : dimension_(dimension > 0 ? dimension : 1),
      defaultMultiplier_(defaultMultiplier),
      multipliers_(static_cast<size_t>(dimension_ * dimension_), defaultMultiplier_),
      dominance_(static_cast<size_t>(dimension_ * dimension_), static_cast<int8_t>(ClashDominance::Neutral)) {}

int InteractionMatrix::index(int row, int col) const { return row * dimension_ + col; }

bool InteractionMatrix::inBounds(int row, int col) const {
    return row >= 0 && col >= 0 && row < dimension_ && col < dimension_;
}

void InteractionMatrix::SetMultiplier(int attackerIdx, int defenderIdx, float multiplier) {
    if (!inBounds(attackerIdx, defenderIdx)) {
        return;
    }
    multipliers_[static_cast<size_t>(index(attackerIdx, defenderIdx))] = multiplier;
}

float InteractionMatrix::GetMultiplier(int attackerIdx, int defenderIdx) const {
    if (!inBounds(attackerIdx, defenderIdx)) {
        return defaultMultiplier_;
    }
    return multipliers_[static_cast<size_t>(index(attackerIdx, defenderIdx))];
}

void InteractionMatrix::SetDominance(int moveA, int moveB, ClashDominance outcome) {
    if (!inBounds(moveA, moveB)) {
        return;
    }
    dominance_[static_cast<size_t>(index(moveA, moveB))] = static_cast<int8_t>(outcome);
}

ClashDominance InteractionMatrix::GetDominance(int moveA, int moveB) const {
    if (!inBounds(moveA, moveB)) {
        return ClashDominance::Neutral;
    }
    return static_cast<ClashDominance>(dominance_[static_cast<size_t>(index(moveA, moveB))]);
}

InteractionMatrix InteractionMatrix::CreateFiveWayPentagon() {
    constexpr int kDimension = 5;
    InteractionMatrix matrix(kDimension, 1.0f);
    for (int attacker = 0; attacker < kDimension; ++attacker) {
        for (int defender = 0; defender < kDimension; ++defender) {
            if (attacker == defender) {
                matrix.SetDominance(attacker, defender, ClashDominance::Neutral);
            } else if (pentagonAttackerWins(attacker, defender, kDimension)) {
                matrix.SetDominance(attacker, defender, ClashDominance::AttackerWins);
            } else {
                matrix.SetDominance(attacker, defender, ClashDominance::DefenderWins);
            }
        }
    }
    return matrix;
}

InteractionMatrix InteractionMatrix::CreateFivePreceptWheel(float winMult, float loseMult) {
    constexpr int kDimension = 5;
    InteractionMatrix matrix(kDimension, 1.0f);
    for (int attacker = 0; attacker < kDimension; ++attacker) {
        for (int defender = 0; defender < kDimension; ++defender) {
            if (attacker == defender) {
                matrix.SetMultiplier(attacker, defender, 1.0f);
            } else if (pentagonAttackerWins(attacker, defender, kDimension)) {
                matrix.SetMultiplier(attacker, defender, winMult);
            } else {
                matrix.SetMultiplier(attacker, defender, loseMult);
            }
        }
    }
    return matrix;
}

}  // namespace CoreEngine