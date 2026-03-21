#pragma once

#include "Genome.h"
#include "BotPlayer.h"
#include <vector>
#include <random>
#include <functional>

namespace CoreEngine {

struct GeneticTrainerConfig {
    int   populationSize  = 20;
    int   matchesPerPair  = 2;
    float mutationRate    = 0.2f;
    float mutationSigma   = 0.3f;
    float weightClamp     = 3.0f;
    int   tournamentSize  = 3;
    int   candidateGrid   = 24;
    unsigned int seed     = 0;
};

/// Generic genetic trainer for evolving BotPlayer genomes.
///
/// Ported from DynamicGO GeneticTrainer, generalized to work with
/// any game that uses RTSGameState + BotPlayer + a PlacementEvaluator.
///
/// The matchFn callback runs a full game between two bots and returns
/// a score in [0, 1] from the perspective of p1 (1.0 = p1 wins).
class GeneticTrainer {
public:
    using MatchFn = std::function<float(const Genome& p1, const Genome& p2)>;
    using Config  = GeneticTrainerConfig;

    explicit GeneticTrainer(Config cfg = Config{});

    void Init();
    void RunGeneration(MatchFn matchFn);

    [[nodiscard]] Genome CrossoverGenomes(const Genome& a, const Genome& b) const;
    [[nodiscard]] Genome MutateGenome(const Genome& g) const;

    [[nodiscard]] const std::vector<Genome>& GetPopulation() const { return population_; }
    [[nodiscard]] int GetGenerationNumber() const { return generation_; }
    [[nodiscard]] Genome GetBest() const { return bestEver_; }

    void SetProgressCallback(std::function<void(int, float)> cb) { progressCallback_ = cb; }

private:
    Config cfg_;
    std::vector<Genome> population_;
    int generation_ = 0;
    Genome bestEver_;
    mutable std::mt19937 rng_;
    std::function<void(int, float)> progressCallback_;

    void RunTournament(MatchFn matchFn);
    Genome SelectParent() const;
    std::vector<Genome> Reproduce() const;
};

} // namespace CoreEngine
