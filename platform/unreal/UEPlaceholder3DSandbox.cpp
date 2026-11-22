#include "platform/unreal/UEPlaceholder3DSandbox.h"
#include "core/ai/Conditions/AlliesNearbyCondition.h"
#include "core/ai/Conditions/EnemyInRangeCondition.h"
#include "core/ai/Conditions/TimeElapsedCondition.h"
#include "core/map/MapComponents/MapObstacleComponent.h"
#include "core/map/MapComponents/MapResourceComponent.h"
#include "core/session/OfflineResolver.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace CoreEngine {
namespace Unreal {

namespace {

/// Live IDecisionSnapshot adapter reading self, nearest enemy, and character attributes.
class UESandboxUnitDecisionSnapshot : public IDecisionSnapshot {
private:
    const RTSUnit* self_ = nullptr;
    const RTSUnit* enemy_ = nullptr;
    const UEGenericCharacter* selfChar_ = nullptr;
    const UEGenericCharacter* enemyChar_ = nullptr;
    uint32_t tick_ = 0;

public:
    UESandboxUnitDecisionSnapshot(const RTSUnit* self,
                                  const RTSUnit* enemy,
                                  const UEGenericCharacter* selfChar,
                                  const UEGenericCharacter* enemyChar,
                                  uint32_t tick)
        : self_(self)
        , enemy_(enemy)
        , selfChar_(selfChar)
        , enemyChar_(enemyChar)
        , tick_(tick) {}

    float GetFloatProperty(DecisionTarget target, const std::string& key,
                           float defaultValue) const override {
        const RTSUnit* u = (target == DecisionTarget::Enemy) ? enemy_ : self_;
        const UEGenericCharacter* c = (target == DecisionTarget::Enemy) ? enemyChar_ : selfChar_;
        if (!u) {
            return defaultValue;
        }
        if (key == "hp") {
            return u->GetCurrentHP();
        }
        if (key == "hp_pct") {
            return (u->GetMaxHP() > 0.0f) ? (u->GetCurrentHP() / u->GetMaxHP()) : 0.0f;
        }
        if (key == "speed") {
            return u->GetVelocity().Length();
        }
        if (key == "stamina_pct" && c) {
            return (c->GetMaxStamina() > 0.0f) ? (c->GetStamina() / c->GetMaxStamina()) : 1.0f;
        }
        if (key == "enemy_distance" && self_ && enemy_) {
            return (self_->GetPosition() - enemy_->GetPosition()).Length();
        }
        return defaultValue;
    }

    int GetIntProperty(DecisionTarget target, const std::string& key,
                       int defaultValue) const override {
        const UEGenericCharacter* c = (target == DecisionTarget::Enemy) ? enemyChar_ : selfChar_;
        if (key == "turn_number" || key == "tick") {
            return static_cast<int>(tick_);
        }
        if (key == "momentum" && c) {
            return c->GetMomentum();
        }
        if (key == "level" && c) {
            return c->GetLevel();
        }
        return defaultValue;
    }

    bool GetBoolProperty(DecisionTarget target, const std::string& key,
                         bool defaultValue) const override {
        const RTSUnit* u = (target == DecisionTarget::Enemy) ? enemy_ : self_;
        const UEGenericCharacter* c = (target == DecisionTarget::Enemy) ? enemyChar_ : selfChar_;
        if (key == "is_dead" && u) {
            return u->IsDead();
        }
        if (key == "has_initiative" && c) {
            return c->GetInitiative() >= 0;
        }
        if (key == "is_exhausted" && c) {
            return c->GetStamina() <= 10.0f;
        }
        return defaultValue;
    }
};

/// Reference IDeterministicMatch implementation backed by RTSGameState for testing
/// Session::InProcessHost offline resolution and online lockstep rollback in UE5.
class UESandboxDeterministicMatch : public Session::IDeterministicMatch {
private:
    std::unique_ptr<RTSGameState> sim_;
    std::vector<uint32_t> playerIds_;
    uint32_t maxTicks_ = 20;
    int32_t team1Score_ = 0;
    int32_t team2Score_ = 0;

public:
    void Reset(uint64_t seed, const std::vector<uint32_t>& playerIds) override {
        BehaviorFactory::RegisterBuiltins();
        playerIds_ = playerIds;
        team1Score_ = 0;
        team2Score_ = 0;
        sim_ = std::make_unique<RTSGameState>(Vector2(800.0f, 600.0f), seed);

        auto u1 = std::make_unique<RTSUnit>(Vector2(200.0f, 300.0f), 1, 14.0f, 110.0f);
        u1->GetStateMachine().AddState("Seek", BehaviorFactory::Create("Seek"));
        u1->GetStateMachine().AddState("Wander", BehaviorFactory::Create("Wander"));
        u1->GetStateMachine().AddState("Flee", BehaviorFactory::Create("Flee"));
        u1->GetStateMachine().SetState("Seek");
        sim_->AddUnit(std::move(u1));

        auto u2 = std::make_unique<RTSUnit>(Vector2(600.0f, 300.0f), 2, 14.0f, 110.0f);
        u2->GetStateMachine().AddState("Seek", BehaviorFactory::Create("Seek"));
        u2->GetStateMachine().AddState("Wander", BehaviorFactory::Create("Wander"));
        u2->GetStateMachine().AddState("Flee", BehaviorFactory::Create("Flee"));
        u2->GetStateMachine().SetState("Seek");
        sim_->AddUnit(std::move(u2));
    }

    void Apply(const Session::PlayerCommand& command) override {
        if (!sim_) {
            return;
        }
        int team = 1;
        if (playerIds_.size() >= 2 && command.playerId == playerIds_[1]) {
            team = 2;
        }
        for (IGameUnit* u : sim_->GetAllUnits()) {
            if (u && u->GetTeam() == team) {
                if (command.action == 1) {
                    sim_->GetCommandSystem().SetUnitState(u, "Seek");
                    if (team == 1) team1Score_ += 2; else team2Score_ += 2;
                } else if (command.action == 2) {
                    sim_->GetCommandSystem().SetUnitState(u, "Wander");
                    if (team == 1) team1Score_ += 1; else team2Score_ += 1;
                } else if (command.action == 3) {
                    sim_->GetCommandSystem().MoveUnitsTo(
                        {u}, Vector2(static_cast<float>(command.x), static_cast<float>(command.y)));
                    if (team == 1) team1Score_ += 3; else team2Score_ += 3;
                }
            }
        }
    }

    void Step() override {
        if (sim_) {
            sim_->Update(1.0f / 30.0f);
        }
    }

    uint64_t StateHash() const override {
        return sim_ ? sim_->CanonicalStateHash() : 0;
    }

    bool IsComplete() const override {
        return sim_ && sim_->GetSimulationTick() >= maxTicks_;
    }

    Session::MatchOutcome Outcome() const override {
        Session::MatchOutcome out;
        out.complete = IsComplete();
        out.forfeit = false;
        out.stateHash = StateHash();
        out.ticksSimulated = sim_ ? sim_->GetSimulationTick() : 0;
        out.scores = {team1Score_, team2Score_};
        if (playerIds_.size() >= 2) {
            if (team1Score_ > team2Score_) {
                out.winnerPlayerId = static_cast<int32_t>(playerIds_[0]);
            } else if (team2Score_ > team1Score_) {
                out.winnerPlayerId = static_cast<int32_t>(playerIds_[1]);
            } else {
                out.winnerPlayerId = -1;
            }
        }
        return out;
    }

    uint32_t MaxTicks() const override { return maxTicks_; }

    std::vector<uint8_t> Snapshot() const override {
        if (!sim_) {
            return {};
        }
        const std::string blob = sim_->CaptureCanonicalState();
        return std::vector<uint8_t>(blob.begin(), blob.end());
    }

    bool Restore(const std::vector<uint8_t>& bytes) override {
        if (!sim_ || bytes.empty()) {
            return false;
        }
        const std::string blob(bytes.begin(), bytes.end());
        return sim_->RestoreCanonicalState(blob);
    }
};

class UESandboxMatchFactory : public Session::IMatchFactory {
public:
    std::unique_ptr<Session::IDeterministicMatch> Create(Session::GameTitle /*game*/) override {
        return std::make_unique<UESandboxDeterministicMatch>();
    }
};

} // namespace

UEPlaceholder3DSandbox::UEPlaceholder3DSandbox(Vector2 arenaSize,
                                               uint64_t seed,
                                               UECoordinateConfig coordConfig)
    : arenaSize_(arenaSize)
    , rngSeed_(seed)
    , logicEngine_(1.0f / 30.0f)
    , team1DeployTimer_(10.0f, 1.0f)
    , team2DeployTimer_(10.0f, 1.0f)
    , team1ThinkTimer_(30.0f)
    , team2ThinkTimer_(30.0f)
    , decisionInterpreter_(100, 0)
    , coordinateBridge_(std::move(coordConfig))
    , renderer_(coordinateBridge_) {
    BehaviorFactory::RegisterBuiltins();
    playback_ = std::make_unique<PlaybackController>(&logicEngine_);

    combatParams_.mode = CombatMode::Hybrid;
    combatParams_.zoneDamagePerSecond = 8.0f;
    combatParams_.proximityDamageMultiplier = 0.5f;
    combatParams_.collisionDamageMultiplier = 1.25f;
    combatParams_.targetPolicy = TargetPolicy::Nearest;

    BuildDefaultDecisionGraph();
    LoadPreset(UESandboxPreset::SingleUnitBehaviorLab, 4);
}

const char* UEPlaceholder3DSandbox::GetPresetName(UESandboxPreset preset) {
    switch (preset) {
        case UESandboxPreset::SingleUnitBehaviorLab: return "SingleUnitBehaviorLab";
        case UESandboxPreset::SingleUnitStateMachineLab: return "SingleUnitStateMachineLab";
        case UESandboxPreset::MultiUnitSquadAndSwarm: return "MultiUnitSquadAndSwarm";
        case UESandboxPreset::CombatProjectilesAndExplosions: return "CombatProjectilesAndExplosions";
        case UESandboxPreset::DecisionGraphAndTurnClashLab: return "DecisionGraphAndTurnClashLab";
        default: return "SingleUnitBehaviorLab";
    }
}

void UEPlaceholder3DSandbox::LoadPreset(UESandboxPreset preset, int unitsPerTeam) {
    BehaviorFactory::RegisterBuiltins();
    activePreset_ = preset;
    configuredUnitsPerTeam_ = std::max(1, unitsPerTeam);

    charactersByEntityId_.clear();
    primaryFocusEntityId_ = 0;
    lastResourceGrants_.clear();
    renderer_.ClearHistory();
    minimap_.ClearPings();

    gameState_ = std::make_unique<RTSGameState>(arenaSize_, rngSeed_);
    gameState_->SetDiagnosticsEnabled(true);
    gameState_->SetPerceptionRadius(140.0f);
    gameState_->AttachCombatManager(&combatManager_);
    gameState_->SetRulesIdentity(std::string("UE5Sandbox:") + GetPresetName(preset));

    // Wire game extension so canonical snapshots capture & restore character inventories
    gameState_->SetGameExtension(
        [this]() -> std::vector<uint8_t> {
            if (primaryFocusEntityId_ != 0) {
                if (const auto* ch = FindCharacterByEntityId(primaryFocusEntityId_)) {
                    return ch->GetInventory().CaptureSnapshotBytes();
                }
            }
            return {};
        },
        [this](const std::vector<uint8_t>& bytes) -> bool {
            if (bytes.empty()) {
                return true;
            }
            UEGenericInventory validator;
            return validator.ValidateSnapshotBytes(bytes);
        },
        [this](const std::vector<uint8_t>& bytes) {
            if (bytes.empty()) {
                return;
            }
            if (primaryFocusEntityId_ != 0) {
                if (auto* ch = FindCharacterByEntityId(primaryFocusEntityId_)) {
                    ch->GetInventory().RestoreSnapshotBytes(bytes);
                    if (RTSUnit* u = FindUnitByEntityId(primaryFocusEntityId_)) {
                        ch->RefreshEffectiveStats(u);
                    }
                }
            }
        });

    logicEngine_.Reset();
    logicEngine_.SetUpdateCallback([this](float dt) { OnFixedSimulationTick(dt); });
    logicEngine_.SetShouldEndCallback([this]() { return matchState_.IsOver(); });

    matchState_ = MatchState{};
    multiTeamMatchState_ = MultiTeamMatchState{};
    multiTeamMatchState_.teamTerritoryPct.assign(3, 0.0f);
    multiTeamMatchState_.teamHoldTime.assign(3, 0.0f);

    team1DeployTimer_ = DeploymentTimer(6.0f, 1.0f);
    team2DeployTimer_ = DeploymentTimer(6.0f, 1.0f);
    team1DeployTimer_.GrantCharge();
    team2DeployTimer_.GrantCharge();
    team1ThinkTimer_ = ThinkTimer(30.0f);
    team2ThinkTimer_ = ThinkTimer(30.0f);

    renderer_.SetGameState(gameState_.get());
    renderer_.SetTerritoryCalculator(&territoryCalculator_);
    minimap_.SetCameraViewportCore(arenaSize_ * 0.5f, arenaSize_ * 0.75f);

    switch (preset) {
        case UESandboxPreset::SingleUnitBehaviorLab: {
            combatEnabled_ = false;
            const uint64_t heroId = SpawnTestUnit(1, Vector2(200.0f, 300.0f), "Seek", true);
            primaryFocusEntityId_ = heroId;
            if (RTSUnit* heroUnit = FindUnitByEntityId(heroId)) {
                gameState_->GetSelectionSystem().SelectUnit(heroUnit, false);
            }
            if (UEGenericCharacter* heroChar = FindCharacterByEntityId(heroId)) {
                heroChar->GetInventory().AddItemById("weapon_plasma_blade", 1);
                heroChar->GetInventory().AddItemById("consumable_nanite_repair", 2);
                heroChar->GetInventory().AddItemById("cartridge_flee", 1);
                heroChar->GetInventory().AddItemById("cartridge_wander", 1);
                heroChar->GetInventory().AddItemById("cartridge_frontline", 1);
            }
            // Stationary enemy target so Seek/Flee/TacticalFrontline have an immediate reference
            const uint64_t dummyId = SpawnTestUnit(2, Vector2(560.0f, 300.0f), "Wander", false);
            if (RTSUnit* dummy = FindUnitByEntityId(dummyId)) {
                dummy->SetMovementParticipation(false);
            }
            gameState_->AddStaticAttractor(
                InfluenceFieldItem::CreateAttractor(Vector2(650.0f, 180.0f), 1.8f));
            AddMapObstacle(Vector2(380.0f, 290.0f), 32.0f, 2.2f);
            AddMapResource("res_center", Vector2(420.0f, 180.0f), 120.0f);
            break;
        }

        case UESandboxPreset::SingleUnitStateMachineLab: {
            combatEnabled_ = false;
            const uint64_t fsmUnitId = SpawnTestUnit(1, Vector2(220.0f, 300.0f), "Wander", true);
            primaryFocusEntityId_ = fsmUnitId;
            ConfigureGenericStateMachine(fsmUnitId, "PatrolEngageFlee");
            if (RTSUnit* u = FindUnitByEntityId(fsmUnitId)) {
                gameState_->GetSelectionSystem().SelectUnit(u, false);
            }
            // Enemy placed within detection range (180 units) to trigger Wander -> Seek transition
            SpawnTestUnit(2, Vector2(340.0f, 300.0f), "Wander", true);
            AddMapObstacle(Vector2(480.0f, 300.0f), 35.0f, 2.0f);
            break;
        }

        case UESandboxPreset::MultiUnitSquadAndSwarm: {
            combatEnabled_ = true;
            combatParams_.mode = CombatMode::InfluenceAttrition;
            combatParams_.zoneDamagePerSecond = 5.0f;
            const int n = configuredUnitsPerTeam_;
            for (int i = 0; i < n; ++i) {
                const float yOffset = 140.0f + static_cast<float>(i) * (320.0f / std::max(1, n - 1));
                const uint64_t t1Id = SpawnTestUnit(1, Vector2(180.0f, yOffset), "Cohesion", (i == 0));
                ConfigureGenericStateMachine(t1Id, "SwarmToFrontline");
                if (i == 0) {
                    primaryFocusEntityId_ = t1Id;
                    if (RTSUnit* u = FindUnitByEntityId(t1Id)) {
                        gameState_->GetSelectionSystem().SelectUnit(u, false);
                    }
                }

                const uint64_t t2Id = SpawnTestUnit(2, Vector2(620.0f, yOffset), "Cohesion", (i == 0));
                ConfigureGenericStateMachine(t2Id, "SwarmToFrontline");
            }
            AddMapObstacle(Vector2(400.0f, 220.0f), 30.0f, 2.0f);
            AddMapObstacle(Vector2(400.0f, 380.0f), 30.0f, 2.0f);
            AddMapResource("res_north", Vector2(400.0f, 100.0f), 150.0f);
            AddMapResource("res_south", Vector2(400.0f, 500.0f), 150.0f);
            break;
        }

        case UESandboxPreset::CombatProjectilesAndExplosions: {
            combatEnabled_ = true;
            combatParams_.mode = CombatMode::Hybrid;
            combatParams_.zoneDamagePerSecond = 10.0f;
            combatParams_.proximityDamageMultiplier = 0.5f;
            const uint64_t u1 = SpawnTestUnit(1, Vector2(240.0f, 260.0f), "TacticalFrontline", true);
            const uint64_t u2 = SpawnTestUnit(1, Vector2(240.0f, 340.0f), "Seek", false);
            const uint64_t e1 = SpawnTestUnit(2, Vector2(520.0f, 260.0f), "TacticalFrontline", true);
            const uint64_t e2 = SpawnTestUnit(2, Vector2(520.0f, 340.0f), "Seek", false);
            (void)u2;
            (void)e1;
            (void)e2;
            primaryFocusEntityId_ = u1;
            if (RTSUnit* primary = FindUnitByEntityId(u1)) {
                gameState_->GetSelectionSystem().SelectUnit(primary, false);
            }
            SpawnTestProjectileFromPrimaryUnit(260.0f, 25.0f);
            break;
        }

        case UESandboxPreset::DecisionGraphAndTurnClashLab: {
            combatEnabled_ = false;
            const uint64_t hero1 = SpawnTestUnit(1, Vector2(280.0f, 300.0f), "Seek", true);
            const uint64_t hero2 = SpawnTestUnit(2, Vector2(500.0f, 300.0f), "Seek", true);
            primaryFocusEntityId_ = hero1;
            if (RTSUnit* u = FindUnitByEntityId(hero1)) {
                gameState_->GetSelectionSystem().SelectUnit(u, false);
            }
            if (UEGenericCharacter* c1 = FindCharacterByEntityId(hero1)) {
                c1->SetArchetypeIndex(0);
                c1->GetInventory().AddItemById("weapon_plasma_blade", 1);
                c1->GetInventory().AddItemById("armor_aegis_plate", 1);
                c1->GetInventory().AddItemById("consumable_nanite_repair", 2);
                c1->EquipItemAndRefresh("weapon_plasma_blade", *gameState_);
            }
            if (UEGenericCharacter* c2 = FindCharacterByEntityId(hero2)) {
                c2->SetArchetypeIndex(1);
                c2->GetInventory().AddItemById("weapon_rail_lance", 1);
                c2->EquipItemAndRefresh("weapon_rail_lance", *gameState_);
            }
            break;
        }
    }

    UpdateTerritoryAndTacticalMetrics(0.0f);
    for (auto& kv : charactersByEntityId_) {
        kv.second->SyncWithSimulation(*gameState_, coordinateBridge_, 0.0f);
    }
    renderer_.Render(0.0f);
    minimap_.UpdateFrame(*gameState_, &territoryCalculator_, coordinateBridge_, 0.0f);
}

void UEPlaceholder3DSandbox::ResetCurrentPreset() {
    LoadPreset(activePreset_, configuredUnitsPerTeam_);
}

void UEPlaceholder3DSandbox::TickFrame(float wallClockDeltaSeconds) {
    if (!gameState_) {
        return;
    }

    inputReader_.PollEvents();
    inputReader_.ProcessWorldInteraction(*gameState_);

    // Active think timers freeze the simulation while draining their real-time bank
    const bool thinkFreeze = team1ThinkTimer_.IsActive() || team2ThinkTimer_.IsActive();
    if (team1ThinkTimer_.IsActive()) {
        team1ThinkTimer_.Update(wallClockDeltaSeconds);
    }
    if (team2ThinkTimer_.IsActive()) {
        team2ThinkTimer_.Update(wallClockDeltaSeconds);
    }

    if (!thinkFreeze) {
        logicEngine_.Update(wallClockDeltaSeconds);
    }

    for (auto& kv : charactersByEntityId_) {
        kv.second->SyncWithSimulation(*gameState_, coordinateBridge_, wallClockDeltaSeconds);
    }

    renderer_.Render(wallClockDeltaSeconds);
    minimap_.UpdateFrame(*gameState_, &territoryCalculator_, coordinateBridge_, wallClockDeltaSeconds);
}

void UEPlaceholder3DSandbox::StepSingleTick() {
    if (!gameState_) {
        return;
    }
    const bool wasSimPaused = gameState_->IsSimulationPaused();
    gameState_->SetSimulationPaused(false);
    OnFixedSimulationTick(logicEngine_.GetFixedTimestep());
    gameState_->SetSimulationPaused(wasSimPaused);

    for (auto& kv : charactersByEntityId_) {
        kv.second->SyncWithSimulation(*gameState_, coordinateBridge_, logicEngine_.GetFixedTimestep());
    }
    renderer_.Render(logicEngine_.GetFixedTimestep());
    minimap_.UpdateFrame(*gameState_, &territoryCalculator_, coordinateBridge_, logicEngine_.GetFixedTimestep());
}

void UEPlaceholder3DSandbox::OnFixedSimulationTick(float fixedDt) {
    if (!gameState_) {
        return;
    }

    team1DeployTimer_.Update(fixedDt);
    team2DeployTimer_.Update(fixedDt);

    // Note pending explosions before RTSGameState::Update clears explosions_ at end of tick
    renderer_.NotePendingExplosions();

    gameState_->Update(fixedDt);

    if (combatEnabled_) {
        combatManager_.Resolve(gameState_->GetAllUnits(), fixedDt, combatParams_);
    }

    UpdateTerritoryAndTacticalMetrics(fixedDt);
}

std::vector<Obstacle> UEPlaceholder3DSandbox::CollectMapObstacles() const {
    std::vector<Obstacle> result;
    if (!gameState_ || !gameState_->GetMap()) {
        return result;
    }
    for (const auto& entityPtr : gameState_->GetMap()->GetEntities()) {
        if (!entityPtr) {
            continue;
        }
        for (const auto& compPtr : entityPtr->GetComponents()) {
            if (const auto* obsComp = dynamic_cast<const MapObstacleComponent*>(compPtr.get())) {
                result.push_back(obsComp->ToObstacle(entityPtr->GetPosition()));
            }
        }
    }
    return result;
}

void UEPlaceholder3DSandbox::UpdateTerritoryAndTacticalMetrics(float fixedDt) {
    if (!gameState_) {
        return;
    }
    const auto allUnits = gameState_->GetAllUnits();
    std::vector<const IGameUnit*> constUnits(allUnits.begin(), allUnits.end());
    const std::vector<Obstacle> obstacles = CollectMapObstacles();
    const TerritoryCalculator::Arena arena{arenaSize_.x, arenaSize_.y};

    // 1. 2-team Voronoi territory calculation & MatchState advance
    const auto terrRes = territoryCalculator_.Calculate(allUnits, arena);
    matchState_.team1TerritoryPct = terrRes.team1Pct / 100.0f;
    matchState_.team2TerritoryPct = terrRes.team2Pct / 100.0f;
    if (fixedDt > 0.0f) {
        matchState_.Advance(fixedDt);
    }

    // 2. N-team influence grid update & MultiTeamMatchState advance
    TerritoryCalculator::GridUpdate gridCfg;
    gridCfg.cols = TerritoryCalculator::GRID_COLS;
    gridCfg.rows = TerritoryCalculator::GRID_ROWS;
    gridCfg.dt = fixedDt;
    gridCfg.captureTimeSeconds = 0.0f;
    territoryCalculator_.UpdateGrid(constUnits, obstacles, arena, gridCfg);

    if (multiTeamMatchState_.teamTerritoryPct.size() < 3) {
        multiTeamMatchState_.teamTerritoryPct.assign(3, 0.0f);
    }
    multiTeamMatchState_.teamTerritoryPct[1] = matchState_.team1TerritoryPct;
    multiTeamMatchState_.teamTerritoryPct[2] = matchState_.team2TerritoryPct;
    if (fixedDt > 0.0f) {
        multiTeamMatchState_.Advance(fixedDt);
    }

    // 3. Tactical influence checkers: ClusterDetector, EncirclementChecker, SurroundChecker
    tacticalSummary_ = UETacticalAnalysisSummary{};
    const auto clusters = ClusterDetector::Detect(
        constUnits, ClusterDetector::kDefaultLinkDistance, false, arenaSize_.x, arenaSize_.y);
    tacticalSummary_.totalClusters = static_cast<int>(clusters.size());

    EncirclementChecker::Query encQuery;
    encQuery.obstacles = obstacles;
    for (const auto& cluster : clusters) {
        if (cluster.empty() || !cluster.front()) {
            continue;
        }
        const int clusterTeam = cluster.front()->GetTeam();
        if (clusterTeam == 1) {
            tacticalSummary_.team1Clusters += 1;
        } else if (clusterTeam == 2) {
            tacticalSummary_.team2Clusters += 1;
        }
        std::vector<const IGameUnit*> enemies;
        for (const IGameUnit* u : constUnits) {
            if (u && u->GetTeam() != clusterTeam) {
                enemies.push_back(u);
            }
        }
        if (EncirclementChecker::IsEncircled(cluster, enemies, arenaSize_.x, arenaSize_.y, encQuery)) {
            tacticalSummary_.encircledClusters += 1;
        }
    }

    for (const IGameUnit* u : constUnits) {
        if (!u) {
            continue;
        }
        if (SurroundChecker::IsSurrounded(u, constUnits, obstacles, arenaSize_.x, arenaSize_.y, false)) {
            tacticalSummary_.surroundedEntityIds.push_back(gameState_->GetEntityId(u));
        }
    }
}

uint64_t UEPlaceholder3DSandbox::SpawnTestUnit(int teamId,
                                               Vector2 corePos,
                                               const std::string& initialBehavior,
                                               bool createCharacterWrapper,
                                               bool requireDeploymentCharge,
                                               bool validatePlacement) {
    if (!gameState_) {
        return 0;
    }

    if (validatePlacement) {
        const auto allUnits = gameState_->GetAllUnits();
        const std::vector<const IGameUnit*> constUnits(allUnits.begin(), allUnits.end());
        const std::vector<Obstacle> obstacles = CollectMapObstacles();
        if (!placementValidator_.IsValidPlacement(
                corePos.x, corePos.y, teamId, constUnits, obstacles,
                arenaSize_.x, arenaSize_.y, placementRules_, 14.0f)) {
            return 0;
        }
    }

    if (requireDeploymentCharge) {
        DeploymentTimer& timer = GetDeploymentTimer(teamId);
        if (!timer.HasCharge()) {
            return 0;
        }
        timer.SpendCharge();
    }

    auto unit = std::make_unique<RTSUnit>(corePos, teamId, 14.0f, 110.0f);
    unit->SetInfluenceRadius(75.0f);
    unit->SetAttackRange(95.0f);
    unit->SetBaseDPS(18.0f);
    unit->SetAttackCooldown(0.75f);

    // Pre-register all built-in behaviors as states so SetState / Magic Dust swap is instant
    for (const std::string& key : BehaviorFactory::GetRegisteredKeys()) {
        auto beh = BehaviorFactory::Create(key);
        if (beh) {
            unit->GetStateMachine().AddState(key, std::move(beh));
        }
    }
    if (!initialBehavior.empty()) {
        if (unit->GetStateMachine().GetStates().find(initialBehavior) == unit->GetStateMachine().GetStates().end()) {
            if (auto customBeh = BehaviorFactory::Create(initialBehavior)) {
                unit->GetStateMachine().AddState(initialBehavior, std::move(customBeh));
            }
        }
        unit->GetStateMachine().SetState(initialBehavior);
    }

    RTSUnit* rawPtr = unit.get();
    gameState_->AddUnit(std::move(unit));
    const uint64_t entityId = gameState_->GetEntityId(rawPtr);

    if (createCharacterWrapper && entityId != 0) {
        std::ostringstream charId;
        charId << "Char_" << entityId;
        std::ostringstream dispName;
        dispName << "Unit #" << entityId << " (Team " << teamId << ")";
        auto character = std::make_unique<UEGenericCharacter>(charId.str(), dispName.str(), teamId);
        character->BindToUnit(*gameState_, rawPtr);
        charactersByEntityId_[entityId] = std::move(character);
        minimap_.MarkHeroEntity(entityId, true);
    }

    if (primaryFocusEntityId_ == 0) {
        primaryFocusEntityId_ = entityId;
    }
    return entityId;
}

RTSUnit* UEPlaceholder3DSandbox::FindUnitByEntityId(uint64_t entityId) {
    if (!gameState_ || entityId == 0) {
        return nullptr;
    }
    for (IGameUnit* u : gameState_->GetAllUnits()) {
        if (u && gameState_->GetEntityId(u) == entityId) {
            return dynamic_cast<RTSUnit*>(u);
        }
    }
    return nullptr;
}

const RTSUnit* UEPlaceholder3DSandbox::FindUnitByEntityId(uint64_t entityId) const {
    if (!gameState_ || entityId == 0) {
        return nullptr;
    }
    for (IGameUnit* u : gameState_->GetAllUnits()) {
        if (u && gameState_->GetEntityId(u) == entityId) {
            return dynamic_cast<const RTSUnit*>(u);
        }
    }
    return nullptr;
}

UEGenericCharacter* UEPlaceholder3DSandbox::FindCharacterByEntityId(uint64_t entityId) {
    auto it = charactersByEntityId_.find(entityId);
    return (it != charactersByEntityId_.end()) ? it->second.get() : nullptr;
}

const UEGenericCharacter* UEPlaceholder3DSandbox::FindCharacterByEntityId(uint64_t entityId) const {
    auto it = charactersByEntityId_.find(entityId);
    return (it != charactersByEntityId_.end()) ? it->second.get() : nullptr;
}

RTSUnit* UEPlaceholder3DSandbox::GetPrimarySelectedOrFocusUnit() {
    if (!gameState_) {
        return nullptr;
    }
    const auto& sel = gameState_->GetSelectionSystem().GetSelectedUnits();
    if (!sel.empty()) {
        if (auto* rts = dynamic_cast<RTSUnit*>(sel.front())) {
            return rts;
        }
    }
    if (primaryFocusEntityId_ != 0) {
        if (auto* rts = FindUnitByEntityId(primaryFocusEntityId_)) {
            return rts;
        }
    }
    for (IGameUnit* u : gameState_->GetAllUnits()) {
        if (auto* rts = dynamic_cast<RTSUnit*>(u)) {
            return rts;
        }
    }
    return nullptr;
}

const RTSUnit* UEPlaceholder3DSandbox::GetPrimarySelectedOrFocusUnit() const {
    return const_cast<UEPlaceholder3DSandbox*>(this)->GetPrimarySelectedOrFocusUnit();
}

bool UEPlaceholder3DSandbox::AssignIndividualBehavior(uint64_t entityId,
                                                      const std::string& behaviorKey) {
    if (!gameState_ || behaviorKey.empty()) {
        return false;
    }

    auto applyToUnit = [&](RTSUnit* unit) -> bool {
        if (!unit) {
            return false;
        }
        StateMachine& fsm = unit->GetStateMachine();
        if (fsm.GetStates().find(behaviorKey) == fsm.GetStates().end()) {
            auto created = BehaviorFactory::Create(behaviorKey);
            if (!created) {
                return false;
            }
            fsm.AddState(behaviorKey, std::move(created));
        }
        fsm.SetState(behaviorKey);
        unit->ClearDebugMode();
        return true;
    };

    if (entityId != 0) {
        return applyToUnit(FindUnitByEntityId(entityId));
    }

    const auto& selected = gameState_->GetSelectionSystem().GetSelectedUnits();
    if (!selected.empty()) {
        bool any = false;
        for (IGameUnit* u : selected) {
            any = applyToUnit(dynamic_cast<RTSUnit*>(u)) || any;
        }
        return any;
    }
    return applyToUnit(GetPrimarySelectedOrFocusUnit());
}

bool UEPlaceholder3DSandbox::ConfigureGenericStateMachine(uint64_t entityId,
                                                          const std::string& fsmPresetName) {
    RTSUnit* unit = (entityId != 0) ? FindUnitByEntityId(entityId) : GetPrimarySelectedOrFocusUnit();
    if (!unit) {
        return false;
    }

    StateMachine& fsm = unit->GetStateMachine();
    fsm = StateMachine(unit);
    for (const std::string& key : BehaviorFactory::GetRegisteredKeys()) {
        if (auto beh = BehaviorFactory::Create(key)) {
            fsm.AddState(key, std::move(beh));
        }
    }

    if (fsmPresetName == "PatrolEngageFlee") {
        fsm.SetState("Wander");
        fsm.AddTransition(StateTransition(
            "Wander", "Seek", std::make_unique<EnemyInRangeCondition>(180.0f), true));
        fsm.AddTransition(StateTransition(
            "Seek", "Flee", std::make_unique<TimeElapsedCondition>(1.5f), true));
        fsm.AddTransition(StateTransition(
            "Flee", "Wander", std::make_unique<TimeElapsedCondition>(1.2f), true));
        return true;
    }

    if (fsmPresetName == "SwarmToFrontline") {
        fsm.SetState("Cohesion");
        fsm.AddTransition(StateTransition(
            "Cohesion", "TacticalFrontline", std::make_unique<AlliesNearbyCondition>(1, 200.0f), true));
        fsm.AddTransition(StateTransition(
            "TacticalFrontline", "Separation", std::make_unique<EnemyInRangeCondition>(50.0f), true));
        fsm.AddTransition(StateTransition(
            "Separation", "TacticalFrontline", std::make_unique<TimeElapsedCondition>(1.0f), true));
        return true;
    }

    if (fsmPresetName == "TimedBehaviorCycle") {
        fsm.SetState("Wander");
        fsm.AddTransition(StateTransition(
            "Wander", "Seek", std::make_unique<TimeElapsedCondition>(1.0f), true));
        fsm.AddTransition(StateTransition(
            "Seek", "Flee", std::make_unique<TimeElapsedCondition>(1.0f), true));
        fsm.AddTransition(StateTransition(
            "Flee", "Wander", std::make_unique<TimeElapsedCondition>(1.0f), true));
        return true;
    }

    return false;
}

bool UEPlaceholder3DSandbox::AddCustomFSMState(uint64_t entityId,
                                               const std::string& stateName,
                                               const std::string& behaviorKey) {
    RTSUnit* unit = (entityId != 0) ? FindUnitByEntityId(entityId) : GetPrimarySelectedOrFocusUnit();
    if (!unit || stateName.empty() || behaviorKey.empty()) {
        return false;
    }
    auto beh = BehaviorFactory::Create(behaviorKey);
    if (!beh) {
        return false;
    }
    unit->GetStateMachine().AddState(stateName, std::move(beh));
    return true;
}

bool UEPlaceholder3DSandbox::AddEnemyInRangeTransition(uint64_t entityId,
                                                       const std::string& fromState,
                                                       const std::string& toState,
                                                       float range,
                                                       bool automatic) {
    RTSUnit* unit = (entityId != 0) ? FindUnitByEntityId(entityId) : GetPrimarySelectedOrFocusUnit();
    if (!unit || fromState.empty() || toState.empty()) {
        return false;
    }
    unit->GetStateMachine().AddTransition(
        StateTransition(fromState, toState, std::make_unique<EnemyInRangeCondition>(range), automatic));
    return true;
}

bool UEPlaceholder3DSandbox::AddAlliesNearbyTransition(uint64_t entityId,
                                                       const std::string& fromState,
                                                       const std::string& toState,
                                                       int minAllies,
                                                       float range,
                                                       bool automatic) {
    RTSUnit* unit = (entityId != 0) ? FindUnitByEntityId(entityId) : GetPrimarySelectedOrFocusUnit();
    if (!unit || fromState.empty() || toState.empty()) {
        return false;
    }
    unit->GetStateMachine().AddTransition(
        StateTransition(fromState, toState,
                        std::make_unique<AlliesNearbyCondition>(minAllies, range), automatic));
    return true;
}

bool UEPlaceholder3DSandbox::AddTimeElapsedTransition(uint64_t entityId,
                                                      const std::string& fromState,
                                                      const std::string& toState,
                                                      float durationSeconds,
                                                      bool automatic) {
    RTSUnit* unit = (entityId != 0) ? FindUnitByEntityId(entityId) : GetPrimarySelectedOrFocusUnit();
    if (!unit || fromState.empty() || toState.empty()) {
        return false;
    }
    unit->GetStateMachine().AddTransition(
        StateTransition(fromState, toState,
                        std::make_unique<TimeElapsedCondition>(durationSeconds), automatic));
    return true;
}

bool UEPlaceholder3DSandbox::TriggerManualStateSwap(uint64_t entityId,
                                                    const std::string& stateName) {
    return AssignIndividualBehavior(entityId, stateName);
}

bool UEPlaceholder3DSandbox::EjectUnitBehaviorState(uint64_t entityId) {
    RTSUnit* unit = (entityId != 0) ? FindUnitByEntityId(entityId) : GetPrimarySelectedOrFocusUnit();
    if (!unit) {
        return false;
    }
    unit->GetStateMachine().ClearCurrentState();
    return true;
}

bool UEPlaceholder3DSandbox::SetUnitBehaviorParameter(uint64_t entityId,
                                                      const std::string& paramName,
                                                      float value) {
    RTSUnit* unit = (entityId != 0) ? FindUnitByEntityId(entityId) : GetPrimarySelectedOrFocusUnit();
    if (!unit || paramName.empty()) {
        return false;
    }
    gameState_->GetCommandSystem().SetBehaviorParameter(unit, paramName, value);
    return true;
}

void UEPlaceholder3DSandbox::BuildDefaultDecisionGraph() {
    decisionGraph_.Clear();

    DecisionNode entry;
    entry.id = "entry";
    entry.type = DecisionNodeType::Entry;
    decisionGraph_.AddNode(entry);

    DecisionNode hpCheck;
    hpCheck.id = "check_low_hp";
    hpCheck.type = DecisionNodeType::Condition;
    hpCheck.target = DecisionTarget::Self;
    hpCheck.valueKind = DecisionValueKind::Float;
    hpCheck.propertyKey = "hp_pct";
    hpCheck.op = ComparisonOp::LT;
    hpCheck.floatThreshold = 0.40f;
    decisionGraph_.AddNode(hpCheck);

    DecisionNode actFlee;
    actFlee.id = "act_flee";
    actFlee.type = DecisionNodeType::Action;
    actFlee.primaryActionId = 1; // 1 = Flee
    actFlee.actionTag = "Flee";
    decisionGraph_.AddNode(actFlee);

    DecisionNode actSeek;
    actSeek.id = "act_seek";
    actSeek.type = DecisionNodeType::Action;
    actSeek.primaryActionId = 2; // 2 = Seek
    actSeek.actionTag = "Seek";
    decisionGraph_.AddNode(actSeek);

    decisionGraph_.AddConnection(DecisionConnection{"entry", 0, "check_low_hp", 0});
    decisionGraph_.AddConnection(DecisionConnection{"check_low_hp", 0, "act_flee", 0}); // True -> Flee
    decisionGraph_.AddConnection(DecisionConnection{"check_low_hp", 1, "act_seek", 0}); // False -> Seek
}

DecisionEvalResult UEPlaceholder3DSandbox::EvaluateDecisionGraphForUnit(uint64_t entityId,
                                                                        bool applyToStateMachine) {
    RTSUnit* selfUnit = (entityId != 0) ? FindUnitByEntityId(entityId) : GetPrimarySelectedOrFocusUnit();
    if (!selfUnit || !gameState_) {
        lastDecisionEval_ = DecisionEvalResult{};
        return lastDecisionEval_;
    }

    const uint64_t selfId = gameState_->GetEntityId(selfUnit);
    const RTSUnit* nearestEnemy = nullptr;
    float bestDistSq = 1e30f;
    for (IGameUnit* candidate : gameState_->GetAllUnits()) {
        if (candidate && candidate->GetTeam() != selfUnit->GetTeam()) {
            const float dSq = (candidate->GetPosition() - selfUnit->GetPosition()).LengthSquared();
            if (dSq < bestDistSq) {
                bestDistSq = dSq;
                nearestEnemy = dynamic_cast<const RTSUnit*>(candidate);
            }
        }
    }

    const UEGenericCharacter* selfChar = FindCharacterByEntityId(selfId);
    const UEGenericCharacter* enemyChar = nearestEnemy
        ? FindCharacterByEntityId(gameState_->GetEntityId(nearestEnemy))
        : nullptr;

    UESandboxUnitDecisionSnapshot snapshot(
        selfUnit, nearestEnemy, selfChar, enemyChar, gameState_->GetSimulationTick());

    lastDecisionEval_ = decisionInterpreter_.Evaluate(
        decisionGraph_, snapshot, gameState_->GetRNG(),
        static_cast<int>(gameState_->GetSimulationTick()));

    if (applyToStateMachine) {
        if (lastDecisionEval_.primaryActionId == 1) {
            AssignIndividualBehavior(selfId, "Flee");
        } else if (lastDecisionEval_.primaryActionId == 2) {
            AssignIndividualBehavior(selfId, "Seek");
        } else if (lastDecisionEval_.primaryActionId == 3) {
            AssignIndividualBehavior(selfId, "TacticalFrontline");
        }
    }

    return lastDecisionEval_;
}

ClashResolutionResult UEPlaceholder3DSandbox::TriggerDemoTurnClash(int initiatorMoveType,
                                                                   int responderMoveType) {
    if (!gameState_) {
        return ClashResolutionResult{};
    }

    UEGenericCharacter* initChar = nullptr;
    UEGenericCharacter* respChar = nullptr;
    for (auto& kv : charactersByEntityId_) {
        if (!initChar && kv.second->GetTeamId() == 1) {
            initChar = kv.second.get();
        } else if (!respChar && kv.second->GetTeamId() != 1) {
            respChar = kv.second.get();
        }
    }
    if (!initChar && !charactersByEntityId_.empty()) {
        initChar = charactersByEntityId_.begin()->second.get();
    }
    if (!respChar && charactersByEntityId_.size() >= 2) {
        auto it = charactersByEntityId_.begin();
        ++it;
        respChar = it->second.get();
    }
    if (!initChar || !respChar) {
        return ClashResolutionResult{};
    }

    lastClashResult_ = initChar->ExecuteTurnClashWith(
        *respChar, initiatorMoveType, responderMoveType, *gameState_, &lastTelegraphResult_);
    return lastClashResult_;
}

uint64_t UEPlaceholder3DSandbox::SpawnTestProjectileFromPrimaryUnit(float speed, float damage) {
    if (!gameState_) {
        return 0;
    }
    RTSUnit* shooter = GetPrimarySelectedOrFocusUnit();
    if (!shooter) {
        return 0;
    }
    const uint64_t shooterId = gameState_->GetEntityId(shooter);

    const IGameUnit* nearestEnemy = nullptr;
    float bestDistSq = 1e30f;
    for (IGameUnit* u : gameState_->GetAllUnits()) {
        if (u && u->GetTeam() != shooter->GetTeam()) {
            const float dSq = (u->GetPosition() - shooter->GetPosition()).LengthSquared();
            if (dSq < bestDistSq) {
                bestDistSq = dSq;
                nearestEnemy = u;
            }
        }
    }

    if (UEGenericCharacter* ch = FindCharacterByEntityId(shooterId)) {
        if (nearestEnemy) {
            return ch->FireInterceptProjectileAtUnit(*gameState_, nearestEnemy, speed, damage, 6.0f, 3.0f);
        }
        return ch->FireProjectileAt(*gameState_, arenaSize_ * 0.5f, speed, damage, 6.0f, 3.0f);
    }

    const Vector2 targetPos = nearestEnemy ? nearestEnemy->GetPosition() : (arenaSize_ * 0.5f);
    Vector2 dir = (targetPos - shooter->GetPosition()).Normalized();
    if (dir.LengthSquared() <= 1e-5f) {
        dir = Vector2(1.0f, 0.0f);
    }
    Projectile p;
    p.ownerId = shooterId;
    p.team = shooter->GetTeam();
    p.position = shooter->GetPosition();
    p.velocity = dir * speed;
    p.radius = 6.0f;
    p.timeRemaining = 3.0f;
    p.damage = damage;
    return gameState_->GetProjectiles().Spawn(p);
}

void UEPlaceholder3DSandbox::TriggerTestExplosion(Vector2 corePos, float radius, float strength) {
    if (!gameState_) {
        return;
    }
    gameState_->AddExplosion(Explosion(corePos, radius, strength, 1.0f));
    renderer_.NotePendingExplosions();
}

void UEPlaceholder3DSandbox::AddMapObstacle(Vector2 corePos, float radius, float repulsionImportance) {
    if (!gameState_ || !gameState_->GetMap()) {
        return;
    }
    std::ostringstream id;
    id << "obs_" << (gameState_->GetMap()->GetEntities().size() + 1);
    auto entity = std::make_unique<MapEntity>(id.str(), corePos);
    InfluenceFieldItem repulsor = InfluenceFieldItem::CreateRepulsor(corePos, repulsionImportance);
    auto comp = std::make_unique<MapObstacleComponent>(radius, repulsor);
    comp->SetObstacleId(static_cast<int>(gameState_->GetMap()->GetEntities().size() + 1));
    entity->AddComponent(std::move(comp));
    gameState_->GetMap()->AddEntity(std::move(entity));
}

void UEPlaceholder3DSandbox::AddMapResource(const std::string& entityId, Vector2 corePos, float amount) {
    if (!gameState_ || !gameState_->GetMap()) {
        return;
    }
    auto entity = std::make_unique<MapEntity>(entityId, corePos);
    auto resComp = std::make_unique<MapResourceComponent>(ResourceType::ENERGY, amount, amount);
    entity->AddComponent(std::move(resComp));
    gameState_->GetMap()->AddEntity(std::move(entity));
}

std::vector<ResourceGrant> UEPlaceholder3DSandbox::ClaimNearestResourcesForAllUnits() {
    lastResourceGrants_.clear();
    if (!gameState_ || !gameState_->GetMap()) {
        return lastResourceGrants_;
    }
    std::vector<ResourceClaim> claims;
    for (IGameUnit* u : gameState_->GetAllUnits()) {
        if (!u) {
            continue;
        }
        const uint64_t unitId = gameState_->GetEntityId(u);
        for (const auto& entPtr : gameState_->GetMap()->GetEntities()) {
            if (!entPtr) {
                continue;
            }
            for (const auto& comp : entPtr->GetComponents()) {
                const auto* res = dynamic_cast<const MapResourceComponent*>(comp.get());
                if (res && !res->IsDepleted()) {
                    claims.push_back(ResourceClaim{unitId, entPtr->GetId()});
                }
            }
        }
    }
    lastResourceGrants_ = ResolveResourceClaims(*gameState_->GetMap(), claims);
    return lastResourceGrants_;
}

bool UEPlaceholder3DSandbox::SaveCanonicalSnapshot(const std::string& slotName) {
    if (!gameState_ || slotName.empty()) {
        return false;
    }
    savedCanonicalSnapshots_[slotName] = gameState_->CaptureCanonicalState();
    return !savedCanonicalSnapshots_[slotName].empty();
}

bool UEPlaceholder3DSandbox::RestoreCanonicalSnapshot(const std::string& slotName) {
    if (!gameState_ || slotName.empty()) {
        return false;
    }
    auto it = savedCanonicalSnapshots_.find(slotName);
    if (it == savedCanonicalSnapshots_.end()) {
        return false;
    }
    const bool ok = gameState_->RestoreCanonicalState(it->second);
    if (ok) {
        UpdateTerritoryAndTacticalMetrics(0.0f);
        for (auto& kv : charactersByEntityId_) {
            kv.second->SyncWithSimulation(*gameState_, coordinateBridge_, 0.0f);
        }
        renderer_.Render(0.0f);
        minimap_.UpdateFrame(*gameState_, &territoryCalculator_, coordinateBridge_, 0.0f);
    }
    return ok;
}

std::string UEPlaceholder3DSandbox::GetDiagnosticJsonString() const {
    if (!gameState_) {
        return "{}";
    }
    GameStateSerializer serializer(gameState_.get());
    return serializer.GetStateJSON();
}

Session::MatchOutcome UEPlaceholder3DSandbox::RunOfflineSessionDemo(uint64_t seed) {
    Session::InProcessHost host(std::make_unique<UESandboxMatchFactory>());
    Session::SessionClient* c1 = host.Connect("Player1_UE5");
    Session::SessionClient* c2 = host.Connect("Player2_UE5");

    c1->CreateLobby(Session::GameTitle::VenusWars, Session::PlayMode::OfflineResolution, 2, seed);
    host.Pump();

    std::string joinCode;
    if (c1->Lobby()) {
        joinCode = c1->Lobby()->joinCode;
    }
    c2->JoinLobby(joinCode);
    host.Pump();

    c1->SetReady(true);
    c2->SetReady(true);
    host.Pump();

    c1->StartMatch();
    host.Pump();

    Session::DecisionPlan p1;
    p1.playerId = c1->PlayerId();
    p1.commands.push_back(Session::PlayerCommand{c1->PlayerId(), 0, 1, 400, 300});
    p1.commands.push_back(Session::PlayerCommand{c1->PlayerId(), 4, 3, 500, 300});

    Session::DecisionPlan p2;
    p2.playerId = c2->PlayerId();
    p2.commands.push_back(Session::PlayerCommand{c2->PlayerId(), 0, 2, 300, 300});

    c1->SubmitPlan(p1);
    c2->SubmitPlan(p2);
    host.Pump();

    Session::MatchOutcome outcome;
    if (c1->Result()) {
        outcome = *c1->Result();
    }

    sessionStatus_.active = true;
    sessionStatus_.gameTitle = Session::GameTitle::VenusWars;
    sessionStatus_.playMode = Session::PlayMode::OfflineResolution;
    sessionStatus_.joinCode = joinCode;
    sessionStatus_.confirmedTick = outcome.ticksSimulated;
    sessionStatus_.confirmedStateHash = outcome.stateHash;
    sessionStatus_.matchComplete = outcome.complete;
    sessionStatus_.winnerPlayerId = outcome.winnerPlayerId;
    sessionStatus_.statusMessage = "OfflineResolution complete";
    return outcome;
}

bool UEPlaceholder3DSandbox::StartOnlineLockstepDemo(uint32_t simulatedRttMs, uint64_t seed) {
    sessionHost_ = std::make_unique<Session::InProcessHost>(std::make_unique<UESandboxMatchFactory>());
    sessionClient1_ = sessionHost_->Connect("UE5_Client1");
    sessionClient2_ = sessionHost_->Connect("UE5_Client2");
    if (!sessionClient1_ || !sessionClient2_) {
        return false;
    }

    // Measure RTT ping/pong
    sessionClient1_->SendPing(1000);
    sessionClient2_->SendPing(1000);
    sessionHost_->Pump();
    sessionClient1_->CompletePing(1000 + simulatedRttMs);
    sessionClient2_->CompletePing(1000 + simulatedRttMs);
    sessionHost_->Pump();

    sessionClient1_->CreateLobby(Session::GameTitle::VenusWars, Session::PlayMode::OnlineSync, 2, seed);
    sessionHost_->Pump();
    if (!sessionClient1_->Lobby()) {
        return false;
    }

    const std::string joinCode = sessionClient1_->Lobby()->joinCode;
    sessionClient2_->JoinLobby(joinCode);
    sessionHost_->Pump();

    sessionClient1_->SetReady(true);
    sessionClient2_->SetReady(true);
    sessionHost_->Pump();

    sessionClient1_->StartMatch();
    sessionHost_->Pump();

    Session::LatencyProfile profile;
    sessionStatus_.active = true;
    sessionStatus_.gameTitle = Session::GameTitle::VenusWars;
    sessionStatus_.playMode = Session::PlayMode::OnlineSync;
    sessionStatus_.simulatedRttMs = simulatedRttMs;
    sessionStatus_.computedInputDelayTicks = Session::InputDelayTicksForRtt(simulatedRttMs, profile);
    sessionStatus_.computedInterpolationDelayMs = Session::InterpolationDelayMs(simulatedRttMs);
    sessionStatus_.joinCode = joinCode;
    sessionStatus_.confirmedTick = 0;
    sessionStatus_.confirmedStateHash = 0;
    sessionStatus_.matchComplete = false;
    sessionStatus_.statusMessage = "OnlineSync active";
    return true;
}

void UEPlaceholder3DSandbox::StepOnlineLockstepDemo(int ticksToAdvance, bool injectPlayerCommands) {
    if (!sessionHost_ || !sessionClient1_ || !sessionClient2_ || ticksToAdvance <= 0) {
        return;
    }
    for (int i = 0; i < ticksToAdvance; ++i) {
        const uint32_t nextTick = sessionClient1_->HasConfirmedTick()
            ? (sessionClient1_->LastConfirmedTick() + 1 + sessionClient1_->InputDelayTicks())
            : sessionStatus_.computedInputDelayTicks;

        if (injectPlayerCommands) {
            sessionClient1_->SubmitInput(
                Session::PlayerCommand{sessionClient1_->PlayerId(), nextTick, 1, 450, 300});
            sessionClient2_->SubmitInput(
                Session::PlayerCommand{sessionClient2_->PlayerId(), nextTick, 2, 350, 300});
            sessionHost_->Pump();
        }

        sessionHost_->AdvanceOnline();
    }

    sessionStatus_.confirmedTick = sessionClient1_->LastConfirmedTick();
    sessionStatus_.confirmedStateHash = sessionClient1_->ConfirmedHash();
    if (sessionClient1_->Result()) {
        sessionStatus_.matchComplete = sessionClient1_->Result()->complete;
        sessionStatus_.winnerPlayerId = sessionClient1_->Result()->winnerPlayerId;
        sessionStatus_.statusMessage = "OnlineSync match complete";
    }
}

} // namespace Unreal
} // namespace CoreEngine
