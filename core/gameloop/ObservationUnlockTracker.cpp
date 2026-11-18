#include "core/gameloop/ObservationUnlockTracker.h"

#include <algorithm>

namespace CoreEngine {

void ObservationUnlockTracker::RegisterSkill(int skillId, int requiredCredits, bool startUnlocked) {
    UnlockProgressEntry entry;
    entry.skillId = skillId;
    entry.requiredCredits = requiredCredits;
    if (startUnlocked) {
        entry.tier = MasteryTier::Novice;
        entry.observationCredits = requiredCredits;
    }
    entries_[skillId] = entry;
}

UnlockProgressEntry* ObservationUnlockTracker::FindEntry(int skillId) {
    const auto it = entries_.find(skillId);
    if (it == entries_.end()) {
        return nullptr;
    }
    return &it->second;
}

const UnlockProgressEntry* ObservationUnlockTracker::FindEntry(int skillId) const {
    const auto it = entries_.find(skillId);
    if (it == entries_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool ObservationUnlockTracker::TryPromoteToNovice(UnlockProgressEntry& entry) {
    if (entry.tier != MasteryTier::Locked) {
        return false;
    }
    if (entry.observationCredits < entry.requiredCredits) {
        return false;
    }
    entry.tier = MasteryTier::Novice;
    return true;
}

bool ObservationUnlockTracker::IntensitySatisfiesGate(const EncounterIntensityMetrics& intensity,
                                                      const IntensityGateConfig& gateConfig) {
    return intensity.turnsSurvived >= gateConfig.minTurns &&
           intensity.enemyHpDamageFraction >= gateConfig.minDamageDealtFraction &&
           intensity.peakAdrenaline >= gateConfig.minPeakAdrenaline &&
           intensity.peakMomentum >= gateConfig.minPeakMomentum;
}

bool ObservationUnlockTracker::AwardObservationCredit(UnlockProgressEntry& entry, int credits) {
    const bool wasLocked = entry.tier == MasteryTier::Locked;
    entry.observationCredits = std::min(entry.observationCredits + credits, entry.requiredCredits);
    const bool newlyUnlocked = wasLocked && TryPromoteToNovice(entry);
    return newlyUnlocked;
}

bool ObservationUnlockTracker::RecordObservation(int skillId, int credits) {
    UnlockProgressEntry* entry = FindEntry(skillId);
    if (entry == nullptr) {
        return false;
    }
    if (entry->tier != MasteryTier::Locked) {
        return false;
    }
    if (observedThisEncounter_.count(skillId) != 0) {
        return false;
    }
    observedThisEncounter_.insert(skillId);
    return AwardObservationCredit(*entry, credits);
}

bool ObservationUnlockTracker::RecordDefeatKillingBlow(int finishingSkillId, const EncounterIntensityMetrics& intensity,
                                                         const IntensityGateConfig& gateConfig) {
    UnlockProgressEntry* entry = FindEntry(finishingSkillId);
    if (entry == nullptr) {
        return false;
    }
    if (entry->tier != MasteryTier::Locked) {
        return false;
    }

    if (IntensitySatisfiesGate(intensity, gateConfig)) {
        entry->observationCredits = entry->requiredCredits;
        entry->tier = MasteryTier::Novice;
        return true;
    }

    if (observedThisEncounter_.count(finishingSkillId) != 0) {
        return false;
    }
    observedThisEncounter_.insert(finishingSkillId);
    return AwardObservationCredit(*entry, 1);
}

MasteryTier ObservationUnlockTracker::RecordSkillUse(int skillId, int uses) {
    UnlockProgressEntry* entry = FindEntry(skillId);
    if (entry == nullptr || entry->tier == MasteryTier::Locked) {
        return entry != nullptr ? entry->tier : MasteryTier::Locked;
    }

    entry->usageCount += uses;

    if (entry->tier == MasteryTier::Novice && entry->usageCount >= entry->adeptUsageTarget) {
        entry->tier = MasteryTier::Adept;
    } else if (entry->tier == MasteryTier::Adept && entry->usageCount >= entry->masterUsageTarget) {
        entry->tier = MasteryTier::Master;
    }

    return entry->tier;
}

void ObservationUnlockTracker::BeginNewEncounter() {
    observedThisEncounter_.clear();
}

bool ObservationUnlockTracker::IsUnlocked(int skillId) const {
    const UnlockProgressEntry* entry = FindEntry(skillId);
    if (entry == nullptr) {
        return false;
    }
    return entry->tier != MasteryTier::Locked;
}

MasteryTier ObservationUnlockTracker::GetMasteryTier(int skillId) const {
    const UnlockProgressEntry* entry = FindEntry(skillId);
    if (entry == nullptr) {
        return MasteryTier::Locked;
    }
    return entry->tier;
}

const UnlockProgressEntry* ObservationUnlockTracker::GetEntry(int skillId) const {
    return FindEntry(skillId);
}

}  // namespace CoreEngine