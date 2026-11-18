#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace CoreEngine {

enum class MasteryTier : uint8_t { Locked = 0, Novice = 1, Adept = 2, Master = 3 };

struct UnlockProgressEntry {
    int skillId = 0;
    MasteryTier tier = MasteryTier::Locked;
    int observationCredits = 0;
    int requiredCredits = 3;
    int usageCount = 0;
    int adeptUsageTarget = 10;
    int masterUsageTarget = 30;
};

struct EncounterIntensityMetrics {
    int turnsSurvived = 0;
    float enemyHpDamageFraction = 0.0f;  // [0.0, 1.0]
    float peakAdrenaline = 0.0f;
    int peakMomentum = 0;
};

struct IntensityGateConfig {
    int minTurns = 4;
    float minDamageDealtFraction = 0.25f;
    float minPeakAdrenaline = 40.0f;
    int minPeakMomentum = 3;
};

class ObservationUnlockTracker {
public:
    void RegisterSkill(int skillId, int requiredCredits = 3, bool startUnlocked = false);

    /// Record witnessing/defending against an enemy skill (capped at 1 credit per skill per encounter by default).
    /// Returns true if the skill newly unlocked (reached requiredCredits).
    bool RecordObservation(int skillId, int credits = 1);

    /// Record a killing blow by an enemy skill on player defeat.
    /// If intensity satisfies gateConfig, unlocks immediately; otherwise awards 1 observation credit. Returns true if unlocked.
    bool RecordDefeatKillingBlow(int finishingSkillId, const EncounterIntensityMetrics& intensity,
                                 const IntensityGateConfig& gateConfig = IntensityGateConfig{});

    /// Record active player use of an unlocked skill; upgrades Novice -> Adept -> Master at thresholds.
    MasteryTier RecordSkillUse(int skillId, int uses = 1);

    void BeginNewEncounter();

    [[nodiscard]] bool IsUnlocked(int skillId) const;
    [[nodiscard]] MasteryTier GetMasteryTier(int skillId) const;
    [[nodiscard]] const UnlockProgressEntry* GetEntry(int skillId) const;

private:
    [[nodiscard]] UnlockProgressEntry* FindEntry(int skillId);
    [[nodiscard]] const UnlockProgressEntry* FindEntry(int skillId) const;
    bool TryPromoteToNovice(UnlockProgressEntry& entry);
    [[nodiscard]] static bool IntensitySatisfiesGate(const EncounterIntensityMetrics& intensity,
                                                     const IntensityGateConfig& gateConfig);
    bool AwardObservationCredit(UnlockProgressEntry& entry, int credits);

    std::unordered_map<int, UnlockProgressEntry> entries_;
    std::unordered_set<int> observedThisEncounter_;
};

}  // namespace CoreEngine