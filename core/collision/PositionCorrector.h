#pragma once

#include "CollisionContact.h"
#include "../interfaces/IGameUnit.h"

class PositionCorrector {
public:
    // Correct position overlap for agent-agent collision
    static void CorrectAgentAgent(
        IGameUnit* unitA,
        IGameUnit* unitB,
        const CollisionContact& contact,
        float correctionFactor,
        float massA,
        float massB
    );

    // Correct position overlap for agent-map collision
    static void CorrectAgentMap(
        IGameUnit* unit,
        const CollisionContact& contact,
        float correctionFactor
    );

    static void ApplyCorrection(IGameUnit* unit, Vector2 correction);
};

