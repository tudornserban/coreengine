#pragma once

#include "core/ai/BehaviorFactory.h"
#include "core/ai/DecisionGraph.h"
#include "core/ai/DecisionGraphInterpreter.h"
#include "core/combat/CombatManager.h"
#include "core/gameloop/DeploymentTimer.h"
#include "core/gameloop/GameLogicEngine.h"
#include "core/gameloop/MatchState.h"
#include "core/gameloop/PlaybackController.h"
#include "core/gameloop/ThinkTimer.h"
#include "core/gamestate/GameStateManager.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/influence/ClusterDetector.h"
#include "core/influence/EncirclementChecker.h"
#include "core/influence/SurroundChecker.h"
#include "core/influence/TerritoryCalculator.h"
#include "core/map/Obstacle.h"
#include "core/map/PlacementValidator.h"
#include "core/map/ResourceClaims.h"
#include "core/serialization/GameStateSerializer.h"
#include "core/session/IDeterministicMatch.h"
#include "core/session/InProcessHost.h"
#include "core/session/Latency.h"
#include "platform/unreal/UECoordinateBridge.h"
#include "platform/unreal/UEGenericCharacter.h"
#include "platform/unreal/UEInputReader.h"
#include "platform/unreal/UEMinimapSystem.h"
#include "platform/unreal/UERenderer.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace CoreEngine {
namespace Unreal {

/// Built-in 3D testing scenarios for 1 unit, N units, FSMs, combat, decision graphs, and clashes.
enum class UESandboxPreset : uint8_t {
    SingleUnitBehaviorLab = 0,
    SingleUnitStateMachineLab = 1,
    MultiUnitSquadAndSwarm = 2,
    CombatProjectilesAndExplosions = 3,
    DecisionGraphAndTurnClashLab = 4
};

/// Live tactical metrics computed from core.influence checkers.
struct UETacticalAnalysisSummary {
    int totalClusters = 0;
    int team1Clusters = 0;
    int team2Clusters = 0;
    int encircledClusters = 0;
    std::vector<uint64_t> surroundedEntityIds;
};

/// Multiplayer session status exposed to the UE5 sandbox & default GUI.
struct UESessionDemoStatus {
    bool active = false;
    Session::GameTitle gameTitle = Session::GameTitle::VenusWars;
    Session::PlayMode playMode = Session::PlayMode::OnlineSync;
    uint32_t simulatedRttMs = 100;
    uint32_t computedInputDelayTicks = 3;
    uint32_t computedInterpolationDelayMs = 50;
    uint32_t confirmedTick = 0;
    uint64_t confirmedStateHash = 0;
    bool matchComplete = false;
    int32_t winnerPlayerId = -1;
    std::string joinCode;
    std::string statusMessage = "Idle";
};

/// Default Placeholder 3D Sandbox for testing individual behaviors, generic state machines
/// (with 1 or N units), characters, inventories, minimap, combat, territory, and sessions in UE5.
class UEPlaceholder3DSandbox {
private:
    Vector2 arenaSize_{800.0f, 600.0f};
    uint64_t rngSeed_ = 42;
    UESandboxPreset activePreset_ = UESandboxPreset::SingleUnitBehaviorLab;
    int configuredUnitsPerTeam_ = 4;

    std::unique_ptr<RTSGameState> gameState_;
    GameLogicEngine logicEngine_;
    std::unique_ptr<PlaybackController> playback_;
    GameStateManager stateManager_;

    CombatManager combatManager_;
    CombatManager::Params combatParams_;
    bool combatEnabled_ = true;

    TerritoryCalculator territoryCalculator_;
    MatchState matchState_;
    MultiTeamMatchState multiTeamMatchState_;
    PlacementValidator placementValidator_;
    PlacementValidator::Rules placementRules_;

    DeploymentTimer team1DeployTimer_;
    DeploymentTimer team2DeployTimer_;
    ThinkTimer team1ThinkTimer_;
    ThinkTimer team2ThinkTimer_;

    DecisionGraph decisionGraph_;
    DecisionGraphInterpreter decisionInterpreter_;
    DecisionEvalResult lastDecisionEval_;

    ClashResolutionResult lastClashResult_;
    TelegraphResult lastTelegraphResult_;

    UETacticalAnalysisSummary tacticalSummary_;
    std::map<std::string, std::string> savedCanonicalSnapshots_;
    std::vector<ResourceGrant> lastResourceGrants_;

    std::map<uint64_t, std::unique_ptr<UEGenericCharacter>> charactersByEntityId_;
    uint64_t primaryFocusEntityId_ = 0;

    UECoordinateBridge coordinateBridge_;
    UERenderer renderer_;
    UEInputReader inputReader_;
    UEMinimapSystem minimap_;

    // Multiplayer session test host
    std::unique_ptr<Session::InProcessHost> sessionHost_;
    Session::SessionClient* sessionClient1_ = nullptr;
    Session::SessionClient* sessionClient2_ = nullptr;
    UESessionDemoStatus sessionStatus_;

public:
    explicit UEPlaceholder3DSandbox(Vector2 arenaSize = Vector2(800.0f, 600.0f),
                                    uint64_t seed = 42,
                                    UECoordinateConfig coordConfig = UECoordinateConfig{});

    // --- Lifecycle & Presets ---
    void LoadPreset(UESandboxPreset preset, int unitsPerTeam = 4);
    void ResetCurrentPreset();
    UESandboxPreset GetActivePreset() const { return activePreset_; }
    static const char* GetPresetName(UESandboxPreset preset);
    int GetConfiguredUnitsPerTeam() const { return configuredUnitsPerTeam_; }

    /// Advance one frame: handles think timers, fixed-timestep simulation ticks,
    /// combat resolution, territory/tactical updates, character sync, 3D rendering, and minimap.
    void TickFrame(float wallClockDeltaSeconds);

    /// Single-step exactly one fixed logic tick (1/30s) even while paused.
    void StepSingleTick();

    // --- Unit, Character & Behavior/FSM Creation & Testing ---
    /// Spawn a unit (and optional UEGenericCharacter wrapper) at corePos.
    /// Returns the new entityId, or 0 if placement/charge validation failed.
    uint64_t SpawnTestUnit(int teamId,
                           Vector2 corePos,
                           const std::string& initialBehavior = "Wander",
                           bool createCharacterWrapper = true,
                           bool requireDeploymentCharge = false,
                           bool validatePlacement = false);

    RTSUnit* FindUnitByEntityId(uint64_t entityId);
    const RTSUnit* FindUnitByEntityId(uint64_t entityId) const;
    UEGenericCharacter* FindCharacterByEntityId(uint64_t entityId);
    const UEGenericCharacter* FindCharacterByEntityId(uint64_t entityId) const;
    const std::map<uint64_t, std::unique_ptr<UEGenericCharacter>>& GetCharacters() const {
        return charactersByEntityId_;
    }

    uint64_t GetPrimaryFocusEntityId() const { return primaryFocusEntityId_; }
    void SetPrimaryFocusEntityId(uint64_t entityId) { primaryFocusEntityId_ = entityId; }
    RTSUnit* GetPrimarySelectedOrFocusUnit();
    const RTSUnit* GetPrimarySelectedOrFocusUnit() const;

    /// Assign a single behavior from BehaviorFactory to a unit (or all selected units if entityId == 0).
    bool AssignIndividualBehavior(uint64_t entityId, const std::string& behaviorKey);

    /// Apply a multi-state generic StateMachine preset to a unit:
    ///   - "PatrolEngageFlee": Wander -> Seek (EnemyInRange) -> Flee (TimeElapsed) -> Wander (TimeElapsed)
    ///   - "SwarmToFrontline": Cohesion -> TacticalFrontline (AlliesNearby) -> Separation (EnemyInRange)
    ///   - "TimedBehaviorCycle": Wander -> Seek -> Flee on timed intervals
    bool ConfigureGenericStateMachine(uint64_t entityId, const std::string& fsmPresetName);

    /// Dynamic FSM builder helpers for custom state machines
    bool AddCustomFSMState(uint64_t entityId, const std::string& stateName, const std::string& behaviorKey);
    bool AddEnemyInRangeTransition(uint64_t entityId, const std::string& fromState,
                                   const std::string& toState, float range, bool automatic = true);
    bool AddAlliesNearbyTransition(uint64_t entityId, const std::string& fromState,
                                   const std::string& toState, int minAllies, float range, bool automatic = true);
    bool AddTimeElapsedTransition(uint64_t entityId, const std::string& fromState,
                                  const std::string& toState, float durationSeconds, bool automatic = true);

    bool TriggerManualStateSwap(uint64_t entityId, const std::string& stateName);
    bool EjectUnitBehaviorState(uint64_t entityId);
    bool SetUnitBehaviorParameter(uint64_t entityId, const std::string& paramName, float value);

    // --- DecisionGraph & TurnClash Testing ---
    void BuildDefaultDecisionGraph();
    DecisionGraph& GetDecisionGraph() { return decisionGraph_; }
    const DecisionGraph& GetDecisionGraph() const { return decisionGraph_; }
    DecisionEvalResult EvaluateDecisionGraphForUnit(uint64_t entityId, bool applyToStateMachine = true);
    const DecisionEvalResult& GetLastDecisionEvalResult() const { return lastDecisionEval_; }

    ClashResolutionResult TriggerDemoTurnClash(int initiatorMoveType = 0, int responderMoveType = 1);
    const ClashResolutionResult& GetLastClashResult() const { return lastClashResult_; }
    const TelegraphResult& GetLastTelegraphResult() const { return lastTelegraphResult_; }

    // --- Combat, Projectiles, Explosions, Map & Resources ---
    uint64_t SpawnTestProjectileFromPrimaryUnit(float speed = 280.0f, float damage = 30.0f);
    void TriggerTestExplosion(Vector2 corePos, float radius = 120.0f, float strength = 4500.0f);
    void AddMapObstacle(Vector2 corePos, float radius = 35.0f, float repulsionImportance = 2.0f);
    void AddMapResource(const std::string& entityId, Vector2 corePos, float amount = 100.0f);
    std::vector<ResourceGrant> ClaimNearestResourcesForAllUnits();
    const std::vector<ResourceGrant>& GetLastResourceGrants() const { return lastResourceGrants_; }

    // --- Snapshots & Serialization ---
    bool SaveCanonicalSnapshot(const std::string& slotName = "quicksave");
    bool RestoreCanonicalSnapshot(const std::string& slotName = "quicksave");
    const std::map<std::string, std::string>& GetSavedSnapshots() const { return savedCanonicalSnapshots_; }
    std::string GetDiagnosticJsonString() const;

    // --- Multiplayer Session Testing (core.session) ---
    Session::MatchOutcome RunOfflineSessionDemo(uint64_t seed = 77);
    bool StartOnlineLockstepDemo(uint32_t simulatedRttMs = 100, uint64_t seed = 99);
    void StepOnlineLockstepDemo(int ticksToAdvance = 1, bool injectPlayerCommands = true);
    const UESessionDemoStatus& GetSessionDemoStatus() const { return sessionStatus_; }

    // --- Subsystem Accessors ---
    RTSGameState& GetGameState() { return *gameState_; }
    const RTSGameState& GetGameState() const { return *gameState_; }
    GameLogicEngine& GetLogicEngine() { return logicEngine_; }
    const GameLogicEngine& GetLogicEngine() const { return logicEngine_; }
    PlaybackController& GetPlaybackController() { return *playback_; }
    const PlaybackController& GetPlaybackController() const { return *playback_; }

    CombatManager& GetCombatManager() { return combatManager_; }
    CombatManager::Params& GetCombatParams() { return combatParams_; }
    const CombatManager::Params& GetCombatParams() const { return combatParams_; }
    bool IsCombatEnabled() const { return combatEnabled_; }
    void SetCombatEnabled(bool enabled) { combatEnabled_ = enabled; }

    TerritoryCalculator& GetTerritoryCalculator() { return territoryCalculator_; }
    const TerritoryCalculator& GetTerritoryCalculator() const { return territoryCalculator_; }
    MatchState& GetMatchState() { return matchState_; }
    const MatchState& GetMatchState() const { return matchState_; }
    MultiTeamMatchState& GetMultiTeamMatchState() { return multiTeamMatchState_; }
    const MultiTeamMatchState& GetMultiTeamMatchState() const { return multiTeamMatchState_; }

    DeploymentTimer& GetDeploymentTimer(int teamId) {
        return (teamId == 2) ? team2DeployTimer_ : team1DeployTimer_;
    }
    const DeploymentTimer& GetDeploymentTimer(int teamId) const {
        return (teamId == 2) ? team2DeployTimer_ : team1DeployTimer_;
    }
    ThinkTimer& GetThinkTimer(int teamId) {
        return (teamId == 2) ? team2ThinkTimer_ : team1ThinkTimer_;
    }
    const ThinkTimer& GetThinkTimer(int teamId) const {
        return (teamId == 2) ? team2ThinkTimer_ : team1ThinkTimer_;
    }

    PlacementValidator& GetPlacementValidator() { return placementValidator_; }
    PlacementValidator::Rules& GetPlacementRules() { return placementRules_; }
    const UETacticalAnalysisSummary& GetTacticalAnalysisSummary() const { return tacticalSummary_; }

    UECoordinateBridge& GetCoordinateBridge() { return coordinateBridge_; }
    const UECoordinateBridge& GetCoordinateBridge() const { return coordinateBridge_; }
    UERenderer& GetRenderer() { return renderer_; }
    const UERenderer& GetRenderer() const { return renderer_; }
    UEInputReader& GetInputReader() { return inputReader_; }
    const UEInputReader& GetInputReader() const { return inputReader_; }
    UEMinimapSystem& GetMinimapSystem() { return minimap_; }
    const UEMinimapSystem& GetMinimapSystem() const { return minimap_; }

private:
    void OnFixedSimulationTick(float fixedDt);
    void UpdateTerritoryAndTacticalMetrics(float fixedDt);
    std::vector<Obstacle> CollectMapObstacles() const;
};

} // namespace Unreal
} // namespace CoreEngine
