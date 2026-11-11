#pragma once

#include <vector>
#include "../ai/BehaviorContext.h"
#include "../math/Vector2.h"
#include "../influence/InfluenceFieldItem.h"

class IGameUnit;
class DeterministicRNG;

struct BehaviorContextInputs {
    IGameUnit* self;
    float deltaTime;
    float perceptionRadius;
    Vector2 mapCenter;
    Vector2 mapSize;
    DeterministicRNG* rng;
    const std::vector<IGameUnit*>& others;
    const std::vector<InfluenceFieldItem>& mapEdges;
    const std::vector<InfluenceFieldItem>& attractors;
    const std::vector<InfluenceFieldItem>& obstacles;
};

BehaviorContext AssembleBehaviorContext(const BehaviorContextInputs& in);
