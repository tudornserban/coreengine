#pragma once

#include <array>
#include <cstdlib>
#include <string>

namespace CoreEngine {

/// Number of features in the genome weight vector.
/// Increase to add more placement evaluation signals.
static constexpr int GENOME_SIZE = 6;

/// Feature weight vector for placement evaluation.
/// Features (indices 0–5 by default):
///   0: territory_gain      — how much territory the placement adds
///   1: safety_margin       — normalised min distance to nearest enemy
///   2: surround_threat     — how many enemies this placement helps surround
///   3: crowding_penalty    — avoids stacking on own units
///   4: center_distance     — preference for center vs edge
///   5: hp_advantage        — own HP vs enemy HP ratio
struct Genome {
    std::array<float, GENOME_SIZE> weights{};
    float fitness = 0.0f;

    static Genome Random(unsigned int seed = 0) {
        if (seed != 0) std::srand(seed);
        Genome g;
        for (float& w : g.weights)
            w = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
        return g;
    }

    /// Serialize to a simple JSON string (no external dependency).
    std::string ToJsonString() const {
        std::string s = "{\"weights\":[";
        for (int i = 0; i < GENOME_SIZE; ++i) {
            s += std::to_string(weights[i]);
            if (i + 1 < GENOME_SIZE) s += ",";
        }
        s += "],\"fitness\":" + std::to_string(fitness) + "}";
        return s;
    }
};

} // namespace CoreEngine
