#pragma once

#include "core/combat/InteractionMatrix.h"
#include "core/math/DeterministicRNG.h"

#include <cstdint>

namespace CoreEngine {

enum class TurnResolutionMode : uint8_t {
    InitiatorResponder = 0, /* Initiative holder acts first with telegraph; Responder can steal initiative */
    Simultaneous = 1        /* Both commit blindly */
};

enum class TelegraphQuality : uint8_t { Unclear = 0, Accurate = 1, FalseBluff = 2 };

struct TelegraphResult {
    TelegraphQuality quality = TelegraphQuality::Unclear;
    int shownMoveType = -1;
    float confidence = 0.0f;
};

struct ClashMoveInput {
    int moveType = 0;
    float baseDamage = 20.0f;
    float staminaCost = 10.0f;
    int momentumGain = 1;
    int initiativeShift = 0;
    float deceptionBonus = 0.0f; /* increases bluff chance */
};

struct ClashConfig {
    TurnResolutionMode mode = TurnResolutionMode::InitiatorResponder;
    float initiatorPowerBonus = 1.15f;
    float neutralDamageScale = 0.40f;
    float momentumScalePerPt = 0.02f;
    int maxMomentumPoints = 10;
};

struct ClashResolutionResult {
    ClashDominance dominance = ClashDominance::Neutral;
    float damageToInitiator = 0.0f;
    float damageToResponder = 0.0f;
    float initiatorStaminaSpent = 0.0f;
    float responderStaminaSpent = 0.0f;
    int initiatorMomentumDelta = 0;
    int responderMomentumDelta = 0;
    bool responderStoleInitiative = false;
    int netInitiativeShift = 0;
};

class TurnClashResolver {
public:
    /// Compute the telegraph signal shown to the Responder after the Initiator picks a move.
    static TelegraphResult ComputeTelegraph(int initiatorMoveType, int numMoveTypes, float responderReadStat,
                                            float initiatorDeceptionStat, DeterministicRNG& rng);

    /// Resolve a clash between Initiator and Responder given the Move Pentagon and Archetype/Precept Wheel.
    static ClashResolutionResult ResolveClash(const ClashMoveInput& initiatorMove, const ClashMoveInput& responderMove,
                                                int initiatorArchetype, int responderArchetype,
                                                int initiatorCurrentMomentum, int responderCurrentMomentum,
                                                const InteractionMatrix& moveDominanceTable,
                                                const InteractionMatrix& archetypeMultiplierTable,
                                                const ClashConfig& config = ClashConfig{});
};

}  // namespace CoreEngine