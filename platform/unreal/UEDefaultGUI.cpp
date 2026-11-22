#include "platform/unreal/UEDefaultGUI.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace CoreEngine {
namespace Unreal {

namespace {

std::string FormatFloat(float v, int decimals = 1) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << v;
    return oss.str();
}

UEGuiWidgetModel MakeSection(const std::string& id, const std::string& title) {
    UEGuiWidgetModel w;
    w.widgetId = id;
    w.kind = UEGuiWidgetKind::SectionHeader;
    w.label = title;
    return w;
}

UEGuiWidgetModel MakeKeyValue(const std::string& id, const std::string& key, const std::string& val) {
    UEGuiWidgetModel w;
    w.widgetId = id;
    w.kind = UEGuiWidgetKind::KeyValueRow;
    w.label = key;
    w.valueText = val;
    return w;
}

UEGuiWidgetModel MakeButton(const std::string& id, const std::string& label, bool enabled = true) {
    UEGuiWidgetModel w;
    w.widgetId = id;
    w.kind = UEGuiWidgetKind::Button;
    w.label = label;
    w.isEnabled = enabled;
    return w;
}

UEGuiWidgetModel MakeToggle(const std::string& id, const std::string& label, bool checked) {
    UEGuiWidgetModel w;
    w.widgetId = id;
    w.kind = UEGuiWidgetKind::ToggleButton;
    w.label = label;
    w.isChecked = checked;
    w.valueText = checked ? "ON" : "OFF";
    return w;
}

UEGuiWidgetModel MakeProgress(const std::string& id, const std::string& label,
                              float progress01, const std::string& text, UEColor color) {
    UEGuiWidgetModel w;
    w.widgetId = id;
    w.kind = UEGuiWidgetKind::ProgressBar;
    w.label = label;
    w.numericValue = std::max(0.0f, std::min(1.0f, progress01));
    w.minValue = 0.0f;
    w.maxValue = 1.0f;
    w.valueText = text;
    w.accentColor = color;
    return w;
}

UEGuiWidgetModel MakeSlider(const std::string& id, const std::string& label,
                            float val, float minV, float maxV) {
    UEGuiWidgetModel w;
    w.widgetId = id;
    w.kind = UEGuiWidgetKind::Slider;
    w.label = label;
    w.numericValue = val;
    w.minValue = minV;
    w.maxValue = maxV;
    w.valueText = FormatFloat(val, 2);
    return w;
}

} // namespace

UEDefaultGUI::UEDefaultGUI(UEPlaceholder3DSandbox* sandbox)
    : sandbox_(sandbox) {
    panelVisibility_[UEGuiPanelId::PlaybackAndMatch] = true;
    panelVisibility_[UEGuiPanelId::BehaviorAndStateMachine] = true;
    panelVisibility_[UEGuiPanelId::UnitCharacterInventory] = true;
    panelVisibility_[UEGuiPanelId::CombatAndClash] = true;
    panelVisibility_[UEGuiPanelId::InfluenceMapCollision] = true;
    panelVisibility_[UEGuiPanelId::SerializationAndSession] = true;
    panelVisibility_[UEGuiPanelId::Minimap] = true;
}

bool UEDefaultGUI::IsPanelVisible(UEGuiPanelId panelId) const {
    auto it = panelVisibility_.find(panelId);
    return (it != panelVisibility_.end()) ? it->second : true;
}

const UEGuiFrameModel& UEDefaultGUI::BuildFrameModel() {
    lastGuiFrame_ = UEGuiFrameModel{};
    if (!sandbox_) {
        lastGuiFrame_.statusBanner = "No Sandbox Attached";
        return lastGuiFrame_;
    }

    lastGuiFrame_.simulationTick = sandbox_->GetGameState().GetSimulationTick();
    lastGuiFrame_.activePresetName = UEPlaceholder3DSandbox::GetPresetName(sandbox_->GetActivePreset());
    lastGuiFrame_.statusBanner = lastStatusBanner_;
    lastGuiFrame_.minimapFrame = sandbox_->GetMinimapSystem().GetLastFrame();

    lastGuiFrame_.panels.push_back(BuildPlaybackAndMatchPanel());
    lastGuiFrame_.panels.push_back(BuildBehaviorAndStateMachinePanel());
    lastGuiFrame_.panels.push_back(BuildUnitCharacterInventoryPanel());
    lastGuiFrame_.panels.push_back(BuildCombatAndClashPanel());
    lastGuiFrame_.panels.push_back(BuildInfluenceMapCollisionPanel());
    lastGuiFrame_.panels.push_back(BuildSerializationAndSessionPanel());
    lastGuiFrame_.panels.push_back(BuildMinimapPanel());

    return lastGuiFrame_;
}

UEGuiPanelModel UEDefaultGUI::BuildPlaybackAndMatchPanel() const {
    UEGuiPanelModel p;
    p.panelId = UEGuiPanelId::PlaybackAndMatch;
    p.title = "1. Simulation, Playback & Match Loop";
    p.visible = IsPanelVisible(p.panelId);
    p.screenX = 16.0f;
    p.screenY = 16.0f;
    p.width = 360.0f;
    p.height = 310.0f;

    const auto& engine = sandbox_->GetLogicEngine();
    const auto& state = sandbox_->GetGameState();
    const auto& match = sandbox_->GetMatchState();

    p.widgets.push_back(MakeSection("sec.presets", "3D Sandbox Scenarios"));
    p.widgets.push_back(MakeButton("preset.single_behavior", "Single-Unit Behavior Lab"));
    p.widgets.push_back(MakeButton("preset.single_fsm", "Single-Unit State Machine Lab"));
    p.widgets.push_back(MakeButton("preset.multi_swarm", "Multi-Unit Squad & Swarm"));
    p.widgets.push_back(MakeButton("preset.combat", "Combat, Projectiles & Explosions"));
    p.widgets.push_back(MakeButton("preset.decision_clash", "Decision Graph & Turn Clash Lab"));

    p.widgets.push_back(MakeSection("sec.playback", "GameLogicEngine & PlaybackController"));
    p.widgets.push_back(MakeToggle("playback.toggle_pause", "Paused", engine.IsPaused() || state.IsSimulationPaused()));
    p.widgets.push_back(MakeButton("playback.step_tick", "Step +1 Tick (1/30s)"));
    p.widgets.push_back(MakeButton("playback.speed_05", "0.5x"));
    p.widgets.push_back(MakeButton("playback.speed_10", "1.0x"));
    p.widgets.push_back(MakeButton("playback.speed_20", "2.0x"));
    p.widgets.push_back(MakeButton("playback.speed_40", "4.0x"));
    p.widgets.push_back(MakeToggle("playback.toggle_ff", "Fast-Forward", engine.IsFastForward()));
    p.widgets.push_back(MakeButton("sandbox.reset", "Reset Scenario"));

    p.widgets.push_back(MakeKeyValue("kv.tick", "Simulation Tick", std::to_string(state.GetSimulationTick())));
    p.widgets.push_back(MakeKeyValue("kv.time", "Game Time (s)", FormatFloat(engine.GetGameTime(), 2)));
    p.widgets.push_back(MakeKeyValue("kv.speed", "Speed Multiplier", FormatFloat(engine.GetSpeedMultiplier(), 2) + "x"));
    p.widgets.push_back(MakeKeyValue("kv.seed", "DeterministicRNG Seed", std::to_string(state.GetRNG().GetSeed())));

    // ThinkTimer & DeploymentTimer
    const auto& t1Deploy = sandbox_->GetDeploymentTimer(1);
    const auto& t2Deploy = sandbox_->GetDeploymentTimer(2);
    const auto& t1Think = sandbox_->GetThinkTimer(1);

    p.widgets.push_back(MakeSection("sec.timers", "Deployment & Think Timers"));
    p.widgets.push_back(MakeProgress("prog.deploy1", "Team 1 Deploy Charges (" + std::to_string(t1Deploy.GetCharges()) + ")",
                                     t1Deploy.GetChargeProgress(), FormatFloat(t1Deploy.GetChargeProgress() * 100.0f, 0) + "%",
                                     UEColor::Team1Blue()));
    p.widgets.push_back(MakeButton("deploy.grant_team1", "+1 Team 1 Charge"));
    p.widgets.push_back(MakeProgress("prog.deploy2", "Team 2 Deploy Charges (" + std::to_string(t2Deploy.GetCharges()) + ")",
                                     t2Deploy.GetChargeProgress(), FormatFloat(t2Deploy.GetChargeProgress() * 100.0f, 0) + "%",
                                     UEColor::Team2Red()));
    p.widgets.push_back(MakeButton("deploy.grant_team2", "+1 Team 2 Charge"));
    p.widgets.push_back(MakeToggle("think.toggle_team1", "Team 1 Tactical Think Pause (" + FormatFloat(t1Think.GetBankRemaining(), 1) + "s)",
                                   t1Think.IsActive()));

    // MatchState
    p.widgets.push_back(MakeSection("sec.match", "MatchState & Territory Dominance"));
    const char* resStr = "Running";
    if (match.result == MatchResult::Team1Wins) resStr = "Team 1 Wins";
    else if (match.result == MatchResult::Team2Wins) resStr = "Team 2 Wins";
    else if (match.result == MatchResult::Draw) resStr = "Draw";
    p.widgets.push_back(MakeKeyValue("kv.match_result", "Match Result", resStr));
    p.widgets.push_back(MakeProgress("prog.match_time", "Match Time",
                                     (match.matchTimeLimit > 0.0f) ? (match.matchTimeElapsed / match.matchTimeLimit) : 0.0f,
                                     FormatFloat(match.matchTimeElapsed, 1) + " / " + FormatFloat(match.matchTimeLimit, 0) + "s",
                                     UEColor::NeutralGray()));
    return p;
}

UEGuiPanelModel UEDefaultGUI::BuildBehaviorAndStateMachinePanel() const {
    UEGuiPanelModel p;
    p.panelId = UEGuiPanelId::BehaviorAndStateMachine;
    p.title = "2. AI Behaviors, State Machine & Decision Graph";
    p.visible = IsPanelVisible(p.panelId);
    p.screenX = 16.0f;
    p.screenY = 338.0f;
    p.width = 360.0f;
    p.height = 360.0f;

    const RTSUnit* focusUnit = sandbox_->GetPrimarySelectedOrFocusUnit();

    p.widgets.push_back(MakeSection("sec.behaviors", "BehaviorFactory (Click to Swap State)"));
    for (const std::string& key : BehaviorFactory::GetRegisteredKeys()) {
        const bool isCurrent = focusUnit && (focusUnit->GetStateMachine().GetCurrentState() == key);
        p.widgets.push_back(MakeToggle("ai.set_behavior:" + key, "Behavior: " + key, isCurrent));
    }
    p.widgets.push_back(MakeButton("ai.eject_state", "Eject / Clear Active Behavior State"));

    p.widgets.push_back(MakeSection("sec.fsm", "Generic StateMachine Presets & Transitions"));
    p.widgets.push_back(MakeButton("fsm.preset:PatrolEngageFlee", "Apply FSM: Patrol -> Engage -> Flee"));
    p.widgets.push_back(MakeButton("fsm.preset:SwarmToFrontline", "Apply FSM: Swarm Cohesion -> Frontline"));
    p.widgets.push_back(MakeButton("fsm.preset:TimedBehaviorCycle", "Apply FSM: Timed Behavior Cycle"));
    p.widgets.push_back(MakeButton("fsm.add_enemy_in_range", "+ Transition: Wander -> Seek (Enemy < 200)"));
    p.widgets.push_back(MakeButton("fsm.add_allies_nearby", "+ Transition: Cohesion -> Frontline (Allies >= 1)"));
    p.widgets.push_back(MakeButton("fsm.add_time_elapsed", "+ Transition: Seek -> Flee (Time >= 2.0s)"));

    if (focusUnit) {
        const StateMachine& fsm = focusUnit->GetStateMachine();
        p.widgets.push_back(MakeKeyValue("kv.fsm_state", "Current FSM State",
                                         fsm.GetCurrentState().empty() ? "<None>" : fsm.GetCurrentState()));
        p.widgets.push_back(MakeKeyValue("kv.fsm_trans_count", "Registered Transitions",
                                         std::to_string(fsm.TransitionCount())));
        for (std::size_t i = 0; i < fsm.TransitionCount(); ++i) {
            if (fsm.TransitionHasNumericState(i)) {
                p.widgets.push_back(MakeKeyValue(
                    "kv.fsm_trans_num_" + std::to_string(i),
                    "Transition #" + std::to_string(i) + " Timer (s)",
                    FormatFloat(fsm.TransitionNumericState(i), 2)));
            }
        }

        if (const IBehavior* beh = fsm.GetCurrentBehavior()) {
            p.widgets.push_back(MakeSection("sec.beh_params", "Live Behavior Parameters"));
            for (const auto& kv : beh->GetParameters()) {
                p.widgets.push_back(MakeSlider("ai.param:" + kv.first, kv.first, kv.second, 0.0f, 500.0f));
            }
        }
    }

    p.widgets.push_back(MakeSection("sec.dgraph", "DecisionGraph & Interpreter"));
    p.widgets.push_back(MakeButton("ai.eval_decision_graph", "Evaluate DecisionGraph on Active Unit"));
    const auto& eval = sandbox_->GetLastDecisionEvalResult();
    p.widgets.push_back(MakeKeyValue("kv.dg_action", "Last Primary Action ID", std::to_string(eval.primaryActionId)));
    p.widgets.push_back(MakeKeyValue("kv.dg_steps", "Graph Nodes Visited", std::to_string(eval.stepsExecuted)));

    return p;
}

UEGuiPanelModel UEDefaultGUI::BuildUnitCharacterInventoryPanel() const {
    UEGuiPanelModel p;
    p.panelId = UEGuiPanelId::UnitCharacterInventory;
    p.title = "3. Unit, Character, Inventory & Timed Actions";
    p.visible = IsPanelVisible(p.panelId);
    p.screenX = 388.0f;
    p.screenY = 16.0f;
    p.width = 370.0f;
    p.height = 380.0f;

    p.widgets.push_back(MakeSection("sec.spawn", "Unit Spawner"));
    p.widgets.push_back(MakeButton("unit.spawn_team1", "Spawn Team 1 Unit + Character"));
    p.widgets.push_back(MakeButton("unit.spawn_team2", "Spawn Team 2 Unit + Character"));
    p.widgets.push_back(MakeKeyValue("kv.unit_count", "Total Units in Arena",
                                     std::to_string(sandbox_->GetGameState().GetAllUnits().size())));

    const RTSUnit* unit = sandbox_->GetPrimarySelectedOrFocusUnit();
    if (unit) {
        const uint64_t entityId = sandbox_->GetGameState().GetEntityId(unit);
        p.widgets.push_back(MakeSection("sec.unit_stats", "Selected RTSUnit (#" + std::to_string(entityId) + ")"));
        p.widgets.push_back(MakeProgress("prog.unit_hp", "Hit Points",
                                         (unit->GetMaxHP() > 0.0f) ? (unit->GetCurrentHP() / unit->GetMaxHP()) : 0.0f,
                                         FormatFloat(unit->GetCurrentHP(), 0) + " / " + FormatFloat(unit->GetMaxHP(), 0),
                                         UEColor::ForTeam(unit->GetTeam())));
        p.widgets.push_back(MakeKeyValue("kv.unit_pos", "Position (Core 2D)",
                                         "(" + FormatFloat(unit->GetPosition().x, 1) + ", " + FormatFloat(unit->GetPosition().y, 1) + ")"));
        p.widgets.push_back(MakeKeyValue("kv.unit_vel", "Velocity",
                                         "(" + FormatFloat(unit->GetVelocity().x, 1) + ", " + FormatFloat(unit->GetVelocity().y, 1) + ")"));
        p.widgets.push_back(MakeKeyValue("kv.unit_mass", "Effective Mass", FormatFloat(unit->CalculateMass(), 2)));
        p.widgets.push_back(MakeSlider("unit.max_speed", "Max Speed", unit->GetMaxSpeed(), 0.0f, 400.0f));
        p.widgets.push_back(MakeSlider("unit.max_accel", "Max Acceleration", unit->GetMaxAcceleration(), 0.0f, 800.0f));
        p.widgets.push_back(MakeSlider("unit.influence_radius", "Influence Radius", unit->GetInfluenceRadius(), 0.0f, 300.0f));
        p.widgets.push_back(MakeSlider("unit.attack_range", "Attack Range", unit->GetAttackRange(), 0.0f, 300.0f));

        p.widgets.push_back(MakeToggle("unit.toggle_movement", "Participates In Movement", unit->ParticipatesInMovement()));
        p.widgets.push_back(MakeToggle("unit.toggle_actions", "Participates In Actions", unit->ParticipatesInActions()));
        p.widgets.push_back(MakeToggle("unit.toggle_collision", "Participates In Collision", unit->ParticipatesInCollision()));

        // TimedActionClock
        p.widgets.push_back(MakeSection("sec.timed_actions", "TimedActionClock"));
        p.widgets.push_back(MakeButton("action.start_30", "Start 30-Tick Timed Action"));
        p.widgets.push_back(MakeButton("action.cancel", "Cancel Active Timed Action"));

        // Character & Inventory
        if (const UEGenericCharacter* ch = sandbox_->FindCharacterByEntityId(entityId)) {
            p.widgets.push_back(MakeSection("sec.char", "UEGenericCharacter: " + ch->GetDisplayName()));
            p.widgets.push_back(MakeKeyValue("kv.char_level", "Level / XP",
                                             "Lv." + std::to_string(ch->GetLevel()) + " (" +
                                             std::to_string(ch->GetExperience()) + "/" +
                                             std::to_string(ch->GetXpToNextLevel()) + ")"));
            p.widgets.push_back(MakeKeyValue("kv.char_anim", "3D Animation State", ch->GetAnimStateName()));
            p.widgets.push_back(MakeProgress("prog.char_stamina", "Stamina",
                                             (ch->GetMaxStamina() > 0.0f) ? (ch->GetStamina() / ch->GetMaxStamina()) : 0.0f,
                                             FormatFloat(ch->GetStamina(), 0) + " / " + FormatFloat(ch->GetMaxStamina(), 0),
                                             UEColor::Team3Green()));
            p.widgets.push_back(MakeButton("char.grant_xp", "+60 Character XP"));

            const auto& inv = ch->GetInventory();
            p.widgets.push_back(MakeSection("sec.inv", "UEGenericInventory (" +
                                            std::to_string(inv.GetUsedSlotCount()) + "/" +
                                            std::to_string(inv.GetMaxSlots()) + " slots, " +
                                            FormatFloat(inv.GetCurrentWeight(), 1) + "/" +
                                            FormatFloat(inv.GetMaxWeight(), 0) + " kg)"));
            for (const auto& slot : inv.GetSlots()) {
                if (slot.IsEmpty()) {
                    continue;
                }
                const std::string& itemId = slot.definition.itemId;
                std::string label = slot.definition.displayName + " x" + std::to_string(slot.quantity);
                if (slot.definition.equipSlot != UEEquipmentSlot::None) {
                    p.widgets.push_back(MakeButton("inv.equip:" + itemId, "Equip " + label));
                }
                if (slot.definition.category == UEItemCategory::Consumable ||
                    slot.definition.category == UEItemCategory::BehaviorCartridge) {
                    p.widgets.push_back(MakeButton("inv.use:" + itemId, "Use " + label));
                }
            }
        }
    }

    return p;
}

UEGuiPanelModel UEDefaultGUI::BuildCombatAndClashPanel() const {
    UEGuiPanelModel p;
    p.panelId = UEGuiPanelId::CombatAndClash;
    p.title = "4. CombatManager, Projectiles, Explosions & Turn Clash";
    p.visible = IsPanelVisible(p.panelId);
    p.screenX = 388.0f;
    p.screenY = 408.0f;
    p.width = 370.0f;
    p.height = 290.0f;

    const auto& params = sandbox_->GetCombatParams();
    p.widgets.push_back(MakeSection("sec.combat_mgr", "CombatManager Resolution"));
    p.widgets.push_back(MakeToggle("combat.toggle_enabled", "Simultaneous Combat Enabled", sandbox_->IsCombatEnabled()));
    p.widgets.push_back(MakeToggle("combat.mode:InfluenceAttrition", "Mode: InfluenceAttrition",
                                   params.mode == CombatMode::InfluenceAttrition));
    p.widgets.push_back(MakeToggle("combat.mode:DirectAttack", "Mode: DirectAttack",
                                   params.mode == CombatMode::DirectAttack));
    p.widgets.push_back(MakeToggle("combat.mode:Hybrid", "Mode: Hybrid",
                                   params.mode == CombatMode::Hybrid));
    p.widgets.push_back(MakeToggle("combat.target:Nearest", "Target: Nearest",
                                   params.targetPolicy == TargetPolicy::Nearest));
    p.widgets.push_back(MakeToggle("combat.target:LowestHP", "Target: LowestHP",
                                   params.targetPolicy == TargetPolicy::LowestHP));
    p.widgets.push_back(MakeToggle("combat.target:HighestThreat", "Target: HighestThreat",
                                   params.targetPolicy == TargetPolicy::HighestThreat));

    p.widgets.push_back(MakeSection("sec.proj_exp", "ProjectileSimulation & Explosions"));
    p.widgets.push_back(MakeButton("combat.fire_projectile", "Fire Intercept Projectile from Active Unit"));
    p.widgets.push_back(MakeButton("combat.trigger_explosion", "Detonate Impulse Explosion at Arena Center"));
    p.widgets.push_back(MakeKeyValue("kv.proj_active", "In-Flight Projectiles",
                                     std::to_string(sandbox_->GetGameState().GetProjectiles().Projectiles().size())));
    p.widgets.push_back(MakeKeyValue("kv.proj_impacts", "Last Tick Projectile Impacts",
                                     std::to_string(sandbox_->GetGameState().GetProjectileImpacts().size())));

    p.widgets.push_back(MakeSection("sec.clash", "InteractionMatrix & TurnClashResolver"));
    p.widgets.push_back(MakeButton("combat.exec_turn_clash", "Resolve 5-Way Pentagon Turn Clash (Attack vs Defend)"));
    const auto& clash = sandbox_->GetLastClashResult();
    p.widgets.push_back(MakeKeyValue("kv.clash_dmg", "Last Clash Damage (Init / Resp)",
                                     FormatFloat(clash.damageToInitiator, 1) + " / " + FormatFloat(clash.damageToResponder, 1)));

    return p;
}

UEGuiPanelModel UEDefaultGUI::BuildInfluenceMapCollisionPanel() const {
    UEGuiPanelModel p;
    p.panelId = UEGuiPanelId::InfluenceMapCollision;
    p.title = "5. 3D Overlays, Territory, Tactical Checkers & Map";
    p.visible = IsPanelVisible(p.panelId);
    p.screenX = 770.0f;
    p.screenY = 16.0f;
    p.width = 360.0f;
    p.height = 310.0f;

    const auto& ov = sandbox_->GetRenderer().GetOverlayFlags();
    p.widgets.push_back(MakeSection("sec.overlays", "3D Scene Overlays (F1-F6)"));
    p.widgets.push_back(MakeToggle("overlay.velocity", "3D Velocity Arrows (F1)", ov.showVelocityArrows));
    p.widgets.push_back(MakeToggle("overlay.propulsion_impulse", "3D Propulsion & Impulse Arrows (F2)", ov.showPropulsionAndImpulse));
    p.widgets.push_back(MakeToggle("overlay.perception", "Perception Radii (F3)", ov.showPerceptionCircles));
    p.widgets.push_back(MakeToggle("overlay.influence", "Influence Field Spheres & Attractors (F4)", ov.showInfluenceField));
    p.widgets.push_back(MakeToggle("overlay.attack_range", "Attack Range Rings (F5)", ov.showAttackRanges));
    p.widgets.push_back(MakeToggle("overlay.territory", "3D Territory Ownership Grid (F6)", ov.showTerritoryGrid));
    p.widgets.push_back(MakeToggle("overlay.state_badges", "Floating 3D State & HP Badges", ov.showStateBadges));

    const auto& tac = sandbox_->GetTacticalAnalysisSummary();
    const auto& match = sandbox_->GetMatchState();
    p.widgets.push_back(MakeSection("sec.tactical", "Territory, Clusters, Encirclement & Surround"));
    p.widgets.push_back(MakeKeyValue("kv.terr_pct", "Voronoi Territory (T1 / T2)",
                                     FormatFloat(match.team1TerritoryPct * 100.0f, 1) + "% / " +
                                     FormatFloat(match.team2TerritoryPct * 100.0f, 1) + "%"));
    p.widgets.push_back(MakeKeyValue("kv.clusters", "Proximity Clusters (Total / T1 / T2)",
                                     std::to_string(tac.totalClusters) + " (" +
                                     std::to_string(tac.team1Clusters) + " / " +
                                     std::to_string(tac.team2Clusters) + ")"));
    p.widgets.push_back(MakeKeyValue("kv.encircled", "Encircled Clusters (Liberty Check)",
                                     std::to_string(tac.encircledClusters)));
    p.widgets.push_back(MakeKeyValue("kv.surrounded", "Surrounded Units (Circumference)",
                                     std::to_string(tac.surroundedEntityIds.size())));

    p.widgets.push_back(MakeSection("sec.map_actions", "Map Entities & Resource Claims"));
    p.widgets.push_back(MakeButton("map.add_obstacle", "Add 3D Obstacle + Repulsor"));
    p.widgets.push_back(MakeButton("map.add_attractor", "Add Static Field Attractor"));
    p.widgets.push_back(MakeButton("map.claim_resources", "Resolve Resource Claims"));
    p.widgets.push_back(MakeKeyValue("kv.grants", "Last Resource Grants",
                                     std::to_string(sandbox_->GetLastResourceGrants().size())));

    return p;
}

UEGuiPanelModel UEDefaultGUI::BuildSerializationAndSessionPanel() const {
    UEGuiPanelModel p;
    p.panelId = UEGuiPanelId::SerializationAndSession;
    p.title = "6. Canonical Snapshots, JSON & Multiplayer Session";
    p.visible = IsPanelVisible(p.panelId);
    p.screenX = 770.0f;
    p.screenY = 338.0f;
    p.width = 360.0f;
    p.height = 220.0f;

    p.widgets.push_back(MakeSection("sec.snapshots", "Canonical Rollback Snapshot & JSON"));
    p.widgets.push_back(MakeButton("snap.save_canonical", "Capture Canonical Snapshot (Quicksave)"));
    p.widgets.push_back(MakeButton("snap.restore_canonical", "Restore Canonical Snapshot (Quickload)"));
    p.widgets.push_back(MakeKeyValue("kv.state_hash", "CanonicalStateHash",
                                     std::to_string(sandbox_->GetGameState().CanonicalStateHash())));

    const auto& sess = sandbox_->GetSessionDemoStatus();
    p.widgets.push_back(MakeSection("sec.session", "core.session (Lobby, Offline & Lockstep)"));
    p.widgets.push_back(MakeButton("session.run_offline", "Run OfflineResolution Match"));
    p.widgets.push_back(MakeButton("session.start_online", "Start OnlineSync Lockstep Match"));
    p.widgets.push_back(MakeButton("session.step_online", "Advance OnlineSync +1 Server Tick"));
    p.widgets.push_back(MakeSlider("session.rtt_ms", "Simulated RTT (ms)",
                                   static_cast<float>(sess.simulatedRttMs), 10.0f, 300.0f));
    p.widgets.push_back(MakeKeyValue("kv.sess_status", "Session Status", sess.statusMessage));
    p.widgets.push_back(MakeKeyValue("kv.sess_delay", "Input Delay / Interp Delay",
                                     std::to_string(sess.computedInputDelayTicks) + " ticks / " +
                                     std::to_string(sess.computedInterpolationDelayMs) + " ms"));
    p.widgets.push_back(MakeKeyValue("kv.sess_tick", "Confirmed Server Tick",
                                     std::to_string(sess.confirmedTick)));
    return p;
}

UEGuiPanelModel UEDefaultGUI::BuildMinimapPanel() const {
    UEGuiPanelModel p;
    p.panelId = UEGuiPanelId::Minimap;
    p.title = "7. Tactical Minimap & Radar";
    p.visible = IsPanelVisible(p.panelId);
    const auto& cfg = sandbox_->GetMinimapSystem().GetConfig();
    p.screenX = cfg.screenOriginX;
    p.screenY = cfg.screenOriginY;
    p.width = cfg.screenWidthPx;
    p.height = cfg.screenHeightPx;

    p.widgets.push_back(MakeToggle("minimap.toggle_heatmap", "Show Territory Heatmap", cfg.showTerritoryHeatmap));
    p.widgets.push_back(MakeButton("minimap.add_ping", "Send Tactical Radar Ping"));
    p.widgets.push_back(MakeKeyValue("kv.minimap_blips", "Tracked Unit Blips",
                                     std::to_string(sandbox_->GetMinimapSystem().GetLastFrame().unitBlips.size())));
    return p;
}

bool UEDefaultGUI::ExecuteButtonAction(const std::string& widgetId) {
    if (!sandbox_) {
        return false;
    }

    // 1. Scenario Presets
    if (widgetId == "preset.single_behavior") {
        sandbox_->LoadPreset(UESandboxPreset::SingleUnitBehaviorLab);
        lastStatusBanner_ = "Loaded Single-Unit Behavior Lab";
        return true;
    }
    if (widgetId == "preset.single_fsm") {
        sandbox_->LoadPreset(UESandboxPreset::SingleUnitStateMachineLab);
        lastStatusBanner_ = "Loaded Single-Unit State Machine Lab";
        return true;
    }
    if (widgetId == "preset.multi_swarm") {
        sandbox_->LoadPreset(UESandboxPreset::MultiUnitSquadAndSwarm);
        lastStatusBanner_ = "Loaded Multi-Unit Squad & Swarm Lab";
        return true;
    }
    if (widgetId == "preset.combat") {
        sandbox_->LoadPreset(UESandboxPreset::CombatProjectilesAndExplosions);
        lastStatusBanner_ = "Loaded Combat, Projectiles & Explosions Lab";
        return true;
    }
    if (widgetId == "preset.decision_clash") {
        sandbox_->LoadPreset(UESandboxPreset::DecisionGraphAndTurnClashLab);
        lastStatusBanner_ = "Loaded Decision Graph & Turn Clash Lab";
        return true;
    }
    if (widgetId == "sandbox.reset") {
        sandbox_->ResetCurrentPreset();
        lastStatusBanner_ = "Reset current scenario preset";
        return true;
    }

    // 2. Playback & Timers
    if (widgetId == "playback.toggle_pause") {
        sandbox_->GetPlaybackController().TogglePause();
        sandbox_->GetGameState().SetSimulationPaused(sandbox_->GetLogicEngine().IsPaused());
        lastStatusBanner_ = sandbox_->GetLogicEngine().IsPaused() ? "Simulation Paused" : "Simulation Resumed";
        return true;
    }
    if (widgetId == "playback.step_tick") {
        sandbox_->StepSingleTick();
        lastStatusBanner_ = "Stepped +1 fixed simulation tick";
        return true;
    }
    if (widgetId == "playback.speed_05") {
        sandbox_->GetPlaybackController().SetSpeed(0.5f);
        lastStatusBanner_ = "Playback speed set to 0.5x";
        return true;
    }
    if (widgetId == "playback.speed_10") {
        sandbox_->GetPlaybackController().SetSpeed(1.0f);
        lastStatusBanner_ = "Playback speed set to 1.0x";
        return true;
    }
    if (widgetId == "playback.speed_20") {
        sandbox_->GetPlaybackController().SetSpeed(2.0f);
        lastStatusBanner_ = "Playback speed set to 2.0x";
        return true;
    }
    if (widgetId == "playback.speed_40") {
        sandbox_->GetPlaybackController().SetSpeed(4.0f);
        lastStatusBanner_ = "Playback speed set to 4.0x";
        return true;
    }
    if (widgetId == "playback.toggle_ff") {
        const bool next = !sandbox_->GetLogicEngine().IsFastForward();
        sandbox_->GetLogicEngine().SetFastForward(next);
        lastStatusBanner_ = next ? "Fast-Forward Enabled" : "Fast-Forward Disabled";
        return true;
    }
    if (widgetId == "deploy.grant_team1") {
        sandbox_->GetDeploymentTimer(1).GrantCharge();
        lastStatusBanner_ = "Granted +1 Deployment Charge to Team 1";
        return true;
    }
    if (widgetId == "deploy.grant_team2") {
        sandbox_->GetDeploymentTimer(2).GrantCharge();
        lastStatusBanner_ = "Granted +1 Deployment Charge to Team 2";
        return true;
    }
    if (widgetId == "think.toggle_team1") {
        ThinkTimer& tt = sandbox_->GetThinkTimer(1);
        if (tt.IsActive()) tt.Deactivate(); else tt.Activate();
        lastStatusBanner_ = tt.IsActive() ? "Team 1 ThinkTimer Activated" : "Team 1 ThinkTimer Deactivated";
        return true;
    }

    // 3. AI Behaviors & StateMachine
    const std::string behPrefix = "ai.set_behavior:";
    if (widgetId.rfind(behPrefix, 0) == 0) {
        const std::string key = widgetId.substr(behPrefix.size());
        const bool ok = sandbox_->AssignIndividualBehavior(0, key);
        if (ok) {
            lastStatusBanner_ = "Assigned behavior: " + key;
        }
        return ok;
    }
    if (widgetId == "ai.eject_state") {
        const bool ok = sandbox_->EjectUnitBehaviorState(0);
        if (ok) {
            lastStatusBanner_ = "Ejected active behavior state";
        }
        return ok;
    }
    const std::string fsmPrefix = "fsm.preset:";
    if (widgetId.rfind(fsmPrefix, 0) == 0) {
        const std::string presetName = widgetId.substr(fsmPrefix.size());
        const bool ok = sandbox_->ConfigureGenericStateMachine(0, presetName);
        if (ok) {
            lastStatusBanner_ = "Configured FSM preset: " + presetName;
        }
        return ok;
    }
    if (widgetId == "fsm.add_enemy_in_range") {
        const bool ok = sandbox_->AddEnemyInRangeTransition(0, "Wander", "Seek", 200.0f, true);
        if (ok) lastStatusBanner_ = "Added EnemyInRangeTransition(Wander -> Seek)";
        return ok;
    }
    if (widgetId == "fsm.add_allies_nearby") {
        const bool ok = sandbox_->AddAlliesNearbyTransition(0, "Cohesion", "TacticalFrontline", 1, 200.0f, true);
        if (ok) lastStatusBanner_ = "Added AlliesNearbyTransition(Cohesion -> TacticalFrontline)";
        return ok;
    }
    if (widgetId == "fsm.add_time_elapsed") {
        const bool ok = sandbox_->AddTimeElapsedTransition(0, "Seek", "Flee", 2.0f, true);
        if (ok) lastStatusBanner_ = "Added TimeElapsedTransition(Seek -> Flee)";
        return ok;
    }
    if (widgetId == "ai.eval_decision_graph") {
        const auto res = sandbox_->EvaluateDecisionGraphForUnit(0, true);
        lastStatusBanner_ = "DecisionGraph chose action ID " + std::to_string(res.primaryActionId);
        return true;
    }

    // 4. Units, Character, Inventory & TimedActions
    if (widgetId == "unit.spawn_team1") {
        const uint64_t id = sandbox_->SpawnTestUnit(1, Vector2(180.0f, 260.0f), "Seek", true);
        lastStatusBanner_ = "Spawned Team 1 Unit #" + std::to_string(id);
        return id != 0;
    }
    if (widgetId == "unit.spawn_team2") {
        const uint64_t id = sandbox_->SpawnTestUnit(2, Vector2(620.0f, 340.0f), "Seek", true);
        lastStatusBanner_ = "Spawned Team 2 Unit #" + std::to_string(id);
        return id != 0;
    }
    if (RTSUnit* unit = sandbox_->GetPrimarySelectedOrFocusUnit()) {
        const uint64_t entityId = sandbox_->GetGameState().GetEntityId(unit);
        if (widgetId == "unit.toggle_movement") {
            unit->SetMovementParticipation(!unit->ParticipatesInMovement());
            return true;
        }
        if (widgetId == "unit.toggle_actions") {
            unit->SetActionParticipation(!unit->ParticipatesInActions());
            return true;
        }
        if (widgetId == "unit.toggle_collision") {
            unit->SetCollisionParticipation(!unit->ParticipatesInCollision());
            return true;
        }
        if (widgetId == "action.start_30") {
            if (auto* ch = sandbox_->FindCharacterByEntityId(entityId)) {
                ch->StartTimedAbility(sandbox_->GetGameState(), 30, 5.0f);
            } else {
                sandbox_->GetGameState().GetTimedActions().Start(entityId, 30);
            }
            lastStatusBanner_ = "Started 30-tick TimedAction";
            return true;
        }
        if (widgetId == "action.cancel") {
            if (auto* ch = sandbox_->FindCharacterByEntityId(entityId)) {
                return ch->CancelTimedAbility(sandbox_->GetGameState());
            }
            return false;
        }
        if (widgetId == "char.grant_xp") {
            if (auto* ch = sandbox_->FindCharacterByEntityId(entityId)) {
                ch->AddExperience(60, &sandbox_->GetGameState());
                lastStatusBanner_ = "Granted +60 XP to " + ch->GetDisplayName();
                return true;
            }
            return false;
        }

        const std::string equipPrefix = "inv.equip:";
        if (widgetId.rfind(equipPrefix, 0) == 0) {
            const std::string itemId = widgetId.substr(equipPrefix.size());
            if (auto* ch = sandbox_->FindCharacterByEntityId(entityId)) {
                const bool ok = ch->EquipItemAndRefresh(itemId, sandbox_->GetGameState());
                if (ok) lastStatusBanner_ = "Equipped item: " + itemId;
                return ok;
            }
            return false;
        }

        const std::string usePrefix = "inv.use:";
        if (widgetId.rfind(usePrefix, 0) == 0) {
            const std::string itemId = widgetId.substr(usePrefix.size());
            if (auto* ch = sandbox_->FindCharacterByEntityId(entityId)) {
                const bool ok = ch->UseInventoryItem(
                    itemId, sandbox_->GetGameState(), &sandbox_->GetDeploymentTimer(unit->GetTeam()));
                if (ok) lastStatusBanner_ = "Used item: " + itemId;
                return ok;
            }
            return false;
        }
    }

    // 5. Combat, Projectiles, Explosions & Turn Clash
    if (widgetId == "combat.toggle_enabled") {
        sandbox_->SetCombatEnabled(!sandbox_->IsCombatEnabled());
        return true;
    }
    if (widgetId == "combat.mode:InfluenceAttrition") {
        sandbox_->GetCombatParams().mode = CombatMode::InfluenceAttrition;
        return true;
    }
    if (widgetId == "combat.mode:DirectAttack") {
        sandbox_->GetCombatParams().mode = CombatMode::DirectAttack;
        return true;
    }
    if (widgetId == "combat.mode:Hybrid") {
        sandbox_->GetCombatParams().mode = CombatMode::Hybrid;
        return true;
    }
    if (widgetId == "combat.target:Nearest") {
        sandbox_->GetCombatParams().targetPolicy = TargetPolicy::Nearest;
        return true;
    }
    if (widgetId == "combat.target:LowestHP") {
        sandbox_->GetCombatParams().targetPolicy = TargetPolicy::LowestHP;
        return true;
    }
    if (widgetId == "combat.target:HighestThreat") {
        sandbox_->GetCombatParams().targetPolicy = TargetPolicy::HighestThreat;
        return true;
    }
    if (widgetId == "combat.fire_projectile") {
        const uint64_t pid = sandbox_->SpawnTestProjectileFromPrimaryUnit(280.0f, 30.0f);
        lastStatusBanner_ = "Fired projectile #" + std::to_string(pid);
        return pid != 0;
    }
    if (widgetId == "combat.trigger_explosion") {
        sandbox_->TriggerTestExplosion(sandbox_->GetGameState().GetMapCenter(), 140.0f, 5000.0f);
        lastStatusBanner_ = "Triggered center explosion impulse";
        return true;
    }
    if (widgetId == "combat.exec_turn_clash") {
        sandbox_->TriggerDemoTurnClash(0, 1);
        lastStatusBanner_ = "Resolved TurnClashResolver 5-Way Pentagon clash";
        return true;
    }

    // 6. Overlays & Map
    auto& ov = sandbox_->GetRenderer().GetOverlayFlags();
    if (widgetId == "overlay.velocity") { ov.showVelocityArrows = !ov.showVelocityArrows; return true; }
    if (widgetId == "overlay.propulsion_impulse") { ov.showPropulsionAndImpulse = !ov.showPropulsionAndImpulse; return true; }
    if (widgetId == "overlay.perception") { ov.showPerceptionCircles = !ov.showPerceptionCircles; return true; }
    if (widgetId == "overlay.influence") { ov.showInfluenceField = !ov.showInfluenceField; return true; }
    if (widgetId == "overlay.attack_range") { ov.showAttackRanges = !ov.showAttackRanges; return true; }
    if (widgetId == "overlay.territory") { ov.showTerritoryGrid = !ov.showTerritoryGrid; return true; }
    if (widgetId == "overlay.state_badges") { ov.showStateBadges = !ov.showStateBadges; return true; }

    if (widgetId == "map.add_obstacle") {
        sandbox_->AddMapObstacle(Vector2(400.0f, 300.0f), 35.0f, 2.0f);
        lastStatusBanner_ = "Added 3D map obstacle";
        return true;
    }
    if (widgetId == "map.add_attractor") {
        sandbox_->GetGameState().AddStaticAttractor(
            InfluenceFieldItem::CreateAttractor(Vector2(600.0f, 200.0f), 2.0f));
        lastStatusBanner_ = "Added static field attractor";
        return true;
    }
    if (widgetId == "map.claim_resources") {
        const auto grants = sandbox_->ClaimNearestResourcesForAllUnits();
        lastStatusBanner_ = "Resolved resource claims (" + std::to_string(grants.size()) + " granted)";
        return true;
    }

    // 7. Snapshots & Session
    if (widgetId == "snap.save_canonical") {
        const bool ok = sandbox_->SaveCanonicalSnapshot("quicksave");
        if (ok) lastStatusBanner_ = "Saved canonical snapshot";
        return ok;
    }
    if (widgetId == "snap.restore_canonical") {
        const bool ok = sandbox_->RestoreCanonicalSnapshot("quicksave");
        if (ok) lastStatusBanner_ = "Restored canonical snapshot";
        return ok;
    }
    if (widgetId == "session.run_offline") {
        const auto out = sandbox_->RunOfflineSessionDemo(77);
        lastStatusBanner_ = "Offline session match complete (hash=" + std::to_string(out.stateHash) + ")";
        return out.complete;
    }
    if (widgetId == "session.start_online") {
        const bool ok = sandbox_->StartOnlineLockstepDemo(sandbox_->GetSessionDemoStatus().simulatedRttMs, 99);
        if (ok) lastStatusBanner_ = "Started OnlineSync lockstep match";
        return ok;
    }
    if (widgetId == "session.step_online") {
        sandbox_->StepOnlineLockstepDemo(1, true);
        lastStatusBanner_ = "Advanced OnlineSync +1 server tick";
        return true;
    }

    // 8. Minimap
    if (widgetId == "minimap.toggle_heatmap") {
        auto& cfg = sandbox_->GetMinimapSystem().GetConfig();
        cfg.showTerritoryHeatmap = !cfg.showTerritoryHeatmap;
        return true;
    }
    if (widgetId == "minimap.add_ping") {
        const Vector2 center = sandbox_->GetGameState().GetMapCenter();
        sandbox_->GetMinimapSystem().AddPing(center, sandbox_->GetGameState().GetMapSize(), 1, "TacticalAlert", 3.0f);
        lastStatusBanner_ = "Added minimap radar ping";
        return true;
    }

    return false;
}

bool UEDefaultGUI::ApplySliderValue(const std::string& widgetId, float value) {
    if (!sandbox_) {
        return false;
    }

    const std::string paramPrefix = "ai.param:";
    if (widgetId.rfind(paramPrefix, 0) == 0) {
        const std::string paramName = widgetId.substr(paramPrefix.size());
        return sandbox_->SetUnitBehaviorParameter(0, paramName, value);
    }

    if (RTSUnit* unit = sandbox_->GetPrimarySelectedOrFocusUnit()) {
        if (widgetId == "unit.max_speed") {
            unit->SetMaxSpeed(std::max(0.0f, value));
            return true;
        }
        if (widgetId == "unit.max_accel") {
            unit->SetMaxAcceleration(std::max(0.0f, value));
            return true;
        }
        if (widgetId == "unit.influence_radius") {
            unit->SetInfluenceRadius(std::max(0.0f, value));
            return true;
        }
        if (widgetId == "unit.attack_range") {
            unit->SetAttackRange(std::max(0.0f, value));
            return true;
        }
    }

    if (widgetId == "session.rtt_ms") {
        const uint32_t rtt = static_cast<uint32_t>(std::max(0.0f, std::round(value)));
        return sandbox_->StartOnlineLockstepDemo(rtt, 99);
    }

    return false;
}

void UEDefaultGUI::ProcessKeyboardShortcuts(UEInputReader& input) {
    if (!sandbox_) {
        return;
    }
    if (input.IsKeyPressed(UE_KEY_SPACE)) {
        ExecuteButtonAction("playback.toggle_pause");
    }
    if (input.IsKeyPressed(UE_KEY_PERIOD)) {
        ExecuteButtonAction("playback.step_tick");
    }
    if (input.IsKeyPressed(UE_KEY_1)) {
        ExecuteButtonAction("playback.speed_05");
    }
    if (input.IsKeyPressed(UE_KEY_2)) {
        ExecuteButtonAction("playback.speed_10");
    }
    if (input.IsKeyPressed(UE_KEY_3)) {
        ExecuteButtonAction("playback.speed_20");
    }
    if (input.IsKeyPressed(UE_KEY_4)) {
        ExecuteButtonAction("playback.speed_40");
    }
    if (input.IsKeyPressed(UE_KEY_R)) {
        ExecuteButtonAction("sandbox.reset");
    }
    if (input.IsKeyPressed(UE_KEY_F1)) {
        ExecuteButtonAction("overlay.velocity");
    }
    if (input.IsKeyPressed(UE_KEY_F2)) {
        ExecuteButtonAction("overlay.propulsion_impulse");
    }
    if (input.IsKeyPressed(UE_KEY_F3)) {
        ExecuteButtonAction("overlay.perception");
    }
    if (input.IsKeyPressed(UE_KEY_F4)) {
        ExecuteButtonAction("overlay.influence");
    }
    if (input.IsKeyPressed(UE_KEY_F5)) {
        ExecuteButtonAction("overlay.attack_range");
    }
    if (input.IsKeyPressed(UE_KEY_F6)) {
        ExecuteButtonAction("overlay.territory");
    }
}

} // namespace Unreal
} // namespace CoreEngine
