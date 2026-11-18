#include "TurnClashResolver.h"

#include <algorithm>
#include <cmath>

namespace CoreEngine {

namespace {

float clamp01(float value) { return std::max(0.0f, std::min(1.0f, value)); }

int clampMomentum(int value, int maxPoints) {
    if (maxPoints <= 0) {
        return 0;
    }
    return std::max(0, std::min(maxPoints, value));
}

float effectiveDamage(float baseDamage, float archetypeMultiplier, int momentum, float momentumScalePerPt, float powerBonus) {
    const float momentumFactor = 1.0f + static_cast<float>(momentum) * momentumScalePerPt;
    return baseDamage * archetypeMultiplier * momentumFactor * powerBonus;
}

}  // namespace

TelegraphResult TurnClashResolver::ComputeTelegraph(int initiatorMoveType, int numMoveTypes, float responderReadStat,
                                                    float initiatorDeceptionStat, DeterministicRNG& rng) {
    TelegraphResult result;
    if (numMoveTypes <= 0) {
        return result;
    }

    const float read = clamp01(responderReadStat);
    const float deception = clamp01(initiatorDeceptionStat);
    const float roll = rng.NextFloat();
    const float bluffSpan = deception * (1.0f - read);

    if (roll < read) {
        result.quality = TelegraphQuality::Accurate;
        result.shownMoveType = initiatorMoveType % numMoveTypes;
        result.confidence = read;
        return result;
    }

    if (roll < read + bluffSpan) {
        result.quality = TelegraphQuality::FalseBluff;
        const int span = std::max(1, numMoveTypes - 1);
        const int offset = 1 + (rng.NextInt32(0, span - 1) % span);
        result.shownMoveType = (initiatorMoveType + offset) % numMoveTypes;
        result.confidence = deception;
        return result;
    }

    result.quality = TelegraphQuality::Unclear;
    result.shownMoveType = -1;
    result.confidence = 0.0f;
    return result;
}

ClashResolutionResult TurnClashResolver::ResolveClash(const ClashMoveInput& initiatorMove,
                                                      const ClashMoveInput& responderMove, int initiatorArchetype,
                                                      int responderArchetype, int initiatorCurrentMomentum,
                                                      int responderCurrentMomentum,
                                                      const InteractionMatrix& moveDominanceTable,
                                                      const InteractionMatrix& archetypeMultiplierTable,
                                                      const ClashConfig& config) {
    ClashResolutionResult result;
    result.dominance =
        moveDominanceTable.GetDominance(initiatorMove.moveType, responderMove.moveType);

    const int clampedInitiatorMomentum =
        clampMomentum(initiatorCurrentMomentum, config.maxMomentumPoints);
    const int clampedResponderMomentum =
        clampMomentum(responderCurrentMomentum, config.maxMomentumPoints);

    const float initiatorArchetypeMult =
        archetypeMultiplierTable.GetMultiplier(initiatorArchetype, responderArchetype);
    const float responderArchetypeMult =
        archetypeMultiplierTable.GetMultiplier(responderArchetype, initiatorArchetype);

    const float initiatorPower =
        (config.mode == TurnResolutionMode::InitiatorResponder) ? config.initiatorPowerBonus : 1.0f;

    const float initiatorOutgoing = effectiveDamage(initiatorMove.baseDamage, initiatorArchetypeMult,
                                                     clampedInitiatorMomentum, config.momentumScalePerPt,
                                                     initiatorPower);
    const float responderOutgoing = effectiveDamage(responderMove.baseDamage, responderArchetypeMult,
                                                    clampedResponderMomentum, config.momentumScalePerPt, 1.0f);

    switch (result.dominance) {
        case ClashDominance::AttackerWins:
            result.damageToResponder = initiatorOutgoing;
            result.damageToInitiator = responderOutgoing * config.neutralDamageScale;
            break;
        case ClashDominance::DefenderWins:
            result.damageToInitiator = responderOutgoing;
            result.damageToResponder = initiatorOutgoing * config.neutralDamageScale;
            if (config.mode == TurnResolutionMode::InitiatorResponder) {
                result.responderStoleInitiative = true;
            }
            break;
        case ClashDominance::Neutral:
        default:
            result.damageToInitiator = initiatorOutgoing * config.neutralDamageScale;
            result.damageToResponder = responderOutgoing * config.neutralDamageScale;
            break;
    }

    result.initiatorStaminaSpent = initiatorMove.staminaCost;
    result.responderStaminaSpent = responderMove.staminaCost;
    result.initiatorMomentumDelta = initiatorMove.momentumGain;
    result.responderMomentumDelta = responderMove.momentumGain;

    int initiativeDelta = responderMove.initiativeShift - initiatorMove.initiativeShift;
    if (result.responderStoleInitiative) {
        initiativeDelta += 1;
    }
    result.netInitiativeShift = initiativeDelta;
    return result;
}

}  // namespace CoreEngine