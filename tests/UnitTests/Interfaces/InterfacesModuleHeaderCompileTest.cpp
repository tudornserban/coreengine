#include <gtest/gtest.h>

#include "core/interfaces/IBehavior.h"
#include "core/interfaces/IGameLogicRenderer.h"
#include "core/interfaces/IGameState.h"
#include "core/interfaces/IGameUnit.h"
#include "core/interfaces/IInfluenceEmitter.h"
#include "core/interfaces/IUnitCombat.h"
#include "core/interfaces/IUnitKinematics.h"
#include "core/interfaces/IUnitAgent.h"
#include "core/interfaces/IInputReader.h"
#include "core/interfaces/IMapComponent.h"
#include "core/interfaces/ISerializer.h"
#include "core/interfaces/ITransitionCondition.h"
#include "core/interfaces/ISimulationSnapshot.h"
#include "core/interfaces/IUnitBehaviorHost.h"
#include "core/interfaces/IDecisionSnapshot.h"
#include "core/interfaces/ITurnCombatant.h"

namespace {

/// Smoke compile: exported interface headers parse with declared foundation deps only.
class InterfacesHeaderCompileTest : public ::testing::Test {};

class MockDecisionSnapshot final : public CoreEngine::IDecisionSnapshot {
public:
    float GetFloatProperty(CoreEngine::DecisionTarget /*target*/, const std::string& /*key*/,
                           float defaultValue) const override {
        return defaultValue;
    }
    int GetIntProperty(CoreEngine::DecisionTarget /*target*/, const std::string& /*key*/,
                       int defaultValue) const override {
        return defaultValue;
    }
    bool GetBoolProperty(CoreEngine::DecisionTarget /*target*/, const std::string& /*key*/,
                         bool defaultValue) const override {
        return defaultValue;
    }
};

class MockTurnCombatant final : public CoreEngine::ITurnCombatant {
public:
    int GetCombatantId() const override { return 1; }
    int GetArchetypeId() const override { return 0; }
    float GetHP() const override { return 50.0f; }
    float GetMaxHP() const override { return 100.0f; }
    float GetStamina() const override { return 10.0f; }
    float GetMaxStamina() const override { return 20.0f; }
    int GetInitiativeMeter() const override { return 3; }
    int GetMomentum() const override { return 0; }
};

}  // namespace

TEST_F(InterfacesHeaderCompileTest, AllExportedHeadersIncluded) {
    SUCCEED();
}

TEST_F(InterfacesHeaderCompileTest, DecisionAndTurnCombatantMocksCompile) {
    MockDecisionSnapshot snapshot;
    EXPECT_FLOAT_EQ(snapshot.GetFloatProperty(CoreEngine::DecisionTarget::Self, "hp", 1.0f), 1.0f);

    MockTurnCombatant combatant;
    EXPECT_EQ(combatant.GetCombatantId(), 1);
    EXPECT_FALSE(combatant.IsDefeated());
}