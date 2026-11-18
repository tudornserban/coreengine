#include <gtest/gtest.h>

#include "core/combat/InteractionMatrix.h"
#include "core/combat/TurnClashResolver.h"
#include "core/math/DeterministicRNG.h"

using CoreEngine::ClashConfig;
using CoreEngine::ClashMoveInput;
using CoreEngine::InteractionMatrix;
using CoreEngine::TelegraphQuality;
using CoreEngine::TurnClashResolver;
using CoreEngine::TurnResolutionMode;

TEST(TurnClashResolverTest, InitiatorPowerBonusIncreasesWinningStrikeDamage) {
    const InteractionMatrix moves = InteractionMatrix::CreateFiveWayPentagon();
    const InteractionMatrix archetypes = InteractionMatrix::CreateFivePreceptWheel();

    ClashMoveInput initiator;
    initiator.moveType = 0;
    initiator.baseDamage = 100.0f;

    ClashMoveInput responder;
    responder.moveType = 1;

    ClashConfig withBonus;
    withBonus.initiatorPowerBonus = 1.15f;

    ClashConfig noBonus;
    noBonus.initiatorPowerBonus = 1.0f;

    const auto boosted = TurnClashResolver::ResolveClash(initiator, responder, 0, 0, 0, 0, moves, archetypes,
                                                          withBonus);
    const auto plain = TurnClashResolver::ResolveClash(initiator, responder, 0, 0, 0, 0, moves, archetypes, noBonus);

    EXPECT_GT(boosted.damageToResponder, plain.damageToResponder);
    EXPECT_FLOAT_EQ(boosted.damageToResponder, plain.damageToResponder * 1.15f);
}

TEST(TurnClashResolverTest, ResponderStealsInitiativeOnDefenderWinInInitiatorMode) {
    const InteractionMatrix moves = InteractionMatrix::CreateFiveWayPentagon();
    const InteractionMatrix archetypes = InteractionMatrix::CreateFivePreceptWheel();

    ClashMoveInput initiator;
    initiator.moveType = 0;
    initiator.baseDamage = 50.0f;
    initiator.initiativeShift = 0;

    ClashMoveInput responder;
    responder.moveType = 3;
    responder.baseDamage = 40.0f;
    responder.initiativeShift = 0;

    ClashConfig config;
    config.mode = TurnResolutionMode::InitiatorResponder;

    const auto result =
        TurnClashResolver::ResolveClash(initiator, responder, 0, 0, 0, 0, moves, archetypes, config);

    EXPECT_TRUE(result.responderStoleInitiative);
    EXPECT_GE(result.netInitiativeShift, 1);
    EXPECT_GT(result.damageToInitiator, result.damageToResponder);
}

TEST(TurnClashResolverTest, TelegraphRollsAreDeterministicAndSupportAccurateAndBluff) {
    DeterministicRNG rngA(4242);
    DeterministicRNG rngB(4242);

    const auto first = TurnClashResolver::ComputeTelegraph(2, 5, 0.9f, 0.1f, rngA);
    const auto second = TurnClashResolver::ComputeTelegraph(2, 5, 0.9f, 0.1f, rngB);
    EXPECT_EQ(first.quality, second.quality);
    EXPECT_EQ(first.shownMoveType, second.shownMoveType);
    EXPECT_FLOAT_EQ(first.confidence, second.confidence);

    DeterministicRNG perfectRead(7);
    const auto accurate = TurnClashResolver::ComputeTelegraph(3, 5, 1.0f, 0.0f, perfectRead);
    EXPECT_EQ(accurate.quality, TelegraphQuality::Accurate);
    EXPECT_EQ(accurate.shownMoveType, 3);

    DeterministicRNG bluffRng(99);
    const auto bluff = TurnClashResolver::ComputeTelegraph(1, 5, 0.0f, 1.0f, bluffRng);
    EXPECT_EQ(bluff.quality, TelegraphQuality::FalseBluff);
    EXPECT_NE(bluff.shownMoveType, 1);
    EXPECT_GE(bluff.shownMoveType, 0);
    EXPECT_LT(bluff.shownMoveType, 5);
}