#include "GeneticTrainer.h"
#include <algorithm>
#include <numeric>

namespace CoreEngine {

GeneticTrainer::GeneticTrainer(Config cfg)
    : cfg_(cfg)
    , rng_((cfg.seed != 0) ? cfg.seed : std::random_device{}())
{}

void GeneticTrainer::Init()
{
    population_.clear();
    population_.reserve(cfg_.populationSize);
    for (int i = 0; i < cfg_.populationSize; ++i)
        population_.push_back(Genome::Random(
            (cfg_.seed != 0) ? cfg_.seed + static_cast<unsigned>(i) : 0));
    generation_ = 0;
}

void GeneticTrainer::RunTournament(MatchFn matchFn)
{
    for (auto& g : population_) g.fitness = 0.0f;

    int n = static_cast<int>(population_.size());
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            for (int m = 0; m < cfg_.matchesPerPair; ++m) {
                float score;
                if (m % 2 == 0)
                    score = matchFn(population_[i], population_[j]);
                else
                    score = 1.0f - matchFn(population_[j], population_[i]);
                population_[i].fitness += score;
                population_[j].fitness += (1.0f - score);
            }
        }
    }
}

Genome GeneticTrainer::SelectParent() const
{
    std::uniform_int_distribution<int> dist(0, static_cast<int>(population_.size()) - 1);
    Genome best = population_[dist(rng_)];
    for (int i = 1; i < cfg_.tournamentSize; ++i) {
        const Genome& candidate = population_[dist(rng_)];
        if (candidate.fitness > best.fitness) best = candidate;
    }
    return best;
}

Genome GeneticTrainer::CrossoverGenomes(const Genome& a, const Genome& b) const
{
    Genome child;
    std::uniform_int_distribution<int> coin(0, 1);
    for (int i = 0; i < GENOME_SIZE; ++i)
        child.weights[i] = (coin(rng_) == 0) ? a.weights[i] : b.weights[i];
    return child;
}

Genome GeneticTrainer::MutateGenome(const Genome& g) const
{
    Genome mutated = g;
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    std::normal_distribution<float>       noise(0.0f, cfg_.mutationSigma);
    for (float& w : mutated.weights) {
        if (prob(rng_) < cfg_.mutationRate) {
            w += noise(rng_);
            w = std::max(-cfg_.weightClamp, std::min(cfg_.weightClamp, w));
        }
    }
    return mutated;
}

std::vector<Genome> GeneticTrainer::Reproduce() const
{
    std::vector<Genome> sorted = population_;
    std::sort(sorted.begin(), sorted.end(),
              [](const Genome& a, const Genome& b){ return a.fitness > b.fitness; });

    int eliteCount = std::max(1, static_cast<int>(sorted.size() / 10));
    std::vector<Genome> next;
    next.reserve(population_.size());
    for (int i = 0; i < eliteCount; ++i) {
        Genome e = sorted[i]; e.fitness = 0.0f; next.push_back(e);
    }

    while (static_cast<int>(next.size()) < static_cast<int>(population_.size())) {
        Genome child = CrossoverGenomes(SelectParent(), SelectParent());
        child = MutateGenome(child);
        child.fitness = 0.0f;
        next.push_back(child);
    }
    return next;
}

void GeneticTrainer::RunGeneration(MatchFn matchFn)
{
    RunTournament(matchFn);

    Genome genBest = *std::max_element(population_.begin(), population_.end(),
        [](const Genome& a, const Genome& b){ return a.fitness < b.fitness; });
    if (genBest.fitness > bestEver_.fitness) bestEver_ = genBest;

    population_ = Reproduce();
    ++generation_;

    if (progressCallback_) progressCallback_(generation_, bestEver_.fitness);
}

} // namespace CoreEngine
