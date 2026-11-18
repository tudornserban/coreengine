#include <gtest/gtest.h>

#include "core/gameloop/ObservationUnlockTracker.h"

using CoreEngine::EncounterIntensityMetrics;
using CoreEngine::IntensityGateConfig;
using CoreEngine::MasteryTier;
using CoreEngine::ObservationUnlockTracker;

TEST(ObservationUnlockTrackerTest, UnlocksAfterThreeObservations) {
    ObservationUnlockTracker tracker;
    constexpr int kSkill = 101;
    tracker.RegisterSkill(kSkill);

    EXPECT_FALSE(tracker.IsUnlocked(kSkill));
    EXPECT_FALSE(tracker.RecordObservation(kSkill));
    EXPECT_EQ(tracker.GetEntry(kSkill)->observationCredits, 1);

    tracker.BeginNewEncounter();
    EXPECT_FALSE(tracker.RecordObservation(kSkill));
    EXPECT_EQ(tracker.GetEntry(kSkill)->observationCredits, 2);

    tracker.BeginNewEncounter();
    EXPECT_TRUE(tracker.RecordObservation(kSkill));
    EXPECT_TRUE(tracker.IsUnlocked(kSkill));
    EXPECT_EQ(tracker.GetMasteryTier(kSkill), MasteryTier::Novice);
    EXPECT_EQ(tracker.GetEntry(kSkill)->observationCredits, 3);
}

TEST(ObservationUnlockTrackerTest, OneObservationCreditPerSkillPerEncounter) {
    ObservationUnlockTracker tracker;
    constexpr int kSkill = 202;
    tracker.RegisterSkill(kSkill);

    EXPECT_FALSE(tracker.RecordObservation(kSkill, 1));
    EXPECT_FALSE(tracker.RecordObservation(kSkill, 1));
    EXPECT_EQ(tracker.GetEntry(kSkill)->observationCredits, 1);

    tracker.BeginNewEncounter();
    EXPECT_FALSE(tracker.RecordObservation(kSkill, 1));
    EXPECT_EQ(tracker.GetEntry(kSkill)->observationCredits, 2);
}

TEST(ObservationUnlockTrackerTest, DefeatIntensityGateInstantUnlockVsShortThrowCreditOnly) {
    ObservationUnlockTracker tracker;
    constexpr int kSkill = 303;
    tracker.RegisterSkill(kSkill);

    IntensityGateConfig gate;
    EncounterIntensityMetrics intense;
    intense.turnsSurvived = 6;
    intense.enemyHpDamageFraction = 0.5f;
    intense.peakAdrenaline = 55.0f;
    intense.peakMomentum = 5;

    EXPECT_TRUE(tracker.RecordDefeatKillingBlow(kSkill, intense, gate));
    EXPECT_TRUE(tracker.IsUnlocked(kSkill));
    EXPECT_EQ(tracker.GetMasteryTier(kSkill), MasteryTier::Novice);

    ObservationUnlockTracker throwTracker;
    constexpr int kThrowSkill = 304;
    throwTracker.RegisterSkill(kThrowSkill);

    EncounterIntensityMetrics shortThrow;
    shortThrow.turnsSurvived = 2;
    shortThrow.enemyHpDamageFraction = 0.1f;
    shortThrow.peakAdrenaline = 10.0f;
    shortThrow.peakMomentum = 0;

    EXPECT_FALSE(throwTracker.RecordDefeatKillingBlow(kThrowSkill, shortThrow, gate));
    EXPECT_FALSE(throwTracker.IsUnlocked(kThrowSkill));
    EXPECT_EQ(throwTracker.GetEntry(kThrowSkill)->observationCredits, 1);
}

TEST(ObservationUnlockTrackerTest, MasteryProgressionNoviceAdeptMaster) {
    ObservationUnlockTracker tracker;
    constexpr int kSkill = 404;
    tracker.RegisterSkill(kSkill, 3, true);

    EXPECT_EQ(tracker.GetMasteryTier(kSkill), MasteryTier::Novice);
    EXPECT_EQ(tracker.RecordSkillUse(kSkill, 9), MasteryTier::Novice);

    EXPECT_EQ(tracker.RecordSkillUse(kSkill, 1), MasteryTier::Adept);
    EXPECT_EQ(tracker.GetMasteryTier(kSkill), MasteryTier::Adept);
    EXPECT_EQ(tracker.GetEntry(kSkill)->usageCount, 10);

    EXPECT_EQ(tracker.RecordSkillUse(kSkill, 19), MasteryTier::Adept);
    EXPECT_EQ(tracker.RecordSkillUse(kSkill, 1), MasteryTier::Master);
    EXPECT_EQ(tracker.GetEntry(kSkill)->usageCount, 30);
}