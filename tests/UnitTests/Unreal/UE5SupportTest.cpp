#include <gtest/gtest.h>

#include "platform/unreal/CoreEngineUE5/Source/CoreEngineUE5/Public/CoreEngineUE5Actors.h"
#include "platform/unreal/CoreEngineUE5/Source/CoreEngineUE5/Public/CoreEngineUE5Module.h"
#include "platform/unreal/UECoordinateBridge.h"
#include "platform/unreal/UEDefaultGUI.h"
#include "platform/unreal/UEGenericCharacter.h"
#include "platform/unreal/UEGenericInventory.h"
#include "platform/unreal/UEInputReader.h"
#include "platform/unreal/UEMinimapSystem.h"
#include "platform/unreal/UEPlaceholder3DSandbox.h"
#include "platform/unreal/UERenderer.h"

using namespace CoreEngine;
using namespace CoreEngine::Unreal;

TEST(UE5SupportTest, CoordinateBridge2D3DRoundTripAndRaycast) {
    UECoordinateConfig cfg;
    cfg.worldScale = 10.0f;
    cfg.originOffset = UEVec3(100.0f, 200.0f, 50.0f);
    cfg.defaultGroundZ = 0.0f;

    UECoordinateBridge bridge(cfg);

    const Vector2 corePos(40.0f, 30.0f);
    const UEVec3 uePos = bridge.CoreToUEPosition(corePos, 25.0f);
    EXPECT_FLOAT_EQ(uePos.x, 500.0f);
    EXPECT_FLOAT_EQ(uePos.y, 500.0f);
    EXPECT_FLOAT_EQ(uePos.z, 75.0f);

    const Vector2 backToCore = bridge.UEToCorePosition(uePos);
    EXPECT_NEAR(backToCore.x, corePos.x, 1e-4f);
    EXPECT_NEAR(backToCore.y, corePos.y, 1e-4f);

    // Custom terrain height callback
    cfg.heightQuery = [](float x, float y) { return (x + y) * 0.5f; };
    bridge.SetConfig(cfg);
    const UEVec3 terrainPos = bridge.CoreToUEPosition(Vector2(20.0f, 40.0f), 10.0f);
    EXPECT_FLOAT_EQ(terrainPos.z, 40.0f); // (20+40)*0.5 + 10

    // Raycast from camera above the arena straight down onto the ground plane (z = 50)
    Vector2 rayHitCore;
    const bool hit = bridge.RaycastGroundToCore(
        UEVec3(500.0f, 500.0f, 1050.0f),
        UEVec3(0.0f, 0.0f, -1.0f),
        &rayHitCore);
    ASSERT_TRUE(hit);
    EXPECT_NEAR(rayHitCore.x, 40.0f, 1e-3f);
    EXPECT_NEAR(rayHitCore.y, 30.0f, 1e-3f);
}

TEST(UE5SupportTest, InputReader3DSelectionAndMoveCommand) {
    BehaviorFactory::RegisterBuiltins();
    RTSGameState state(Vector2(800.0f, 600.0f), 1);
    auto unit = std::make_unique<RTSUnit>(Vector2(200.0f, 300.0f), 1, 15.0f, 100.0f);
    RTSUnit* rawUnit = unit.get();
    state.AddUnit(std::move(unit));

    UECoordinateBridge bridge;
    UEInputReader input;

    // 1. Raycast left-click on the unit at (200, 300) -> UE (2000, 3000, 0)
    ASSERT_TRUE(input.InjectWorldCameraRay(
        UEVec3(2000.0f, 3000.0f, 1000.0f), UEVec3(0.0f, 0.0f, -1.0f), bridge));
    input.InjectMouseButtonState(UE_MOUSE_LEFT, true);
    input.PollEvents();
    EXPECT_TRUE(input.IsMouseButtonPressed(UE_MOUSE_LEFT));
    input.ProcessWorldInteraction(state);

    input.InjectMouseButtonState(UE_MOUSE_LEFT, false);
    input.PollEvents();
    EXPECT_TRUE(input.IsMouseButtonReleased(UE_MOUSE_LEFT));
    input.ProcessWorldInteraction(state);

    EXPECT_TRUE(rawUnit->IsSelected());

    // 2. Right-click at (500, 350) to command movement
    input.InjectWorldCoreCursor(Vector2(500.0f, 350.0f));
    input.InjectMouseButtonState(UE_MOUSE_RIGHT, true);
    input.PollEvents();
    input.ProcessWorldInteraction(state);

    EXPECT_TRUE(rawUnit->IsInDebugMode());
}

TEST(UE5SupportTest, RendererBuilds3DPlaceholderProxiesAndExplosionEchoes) {
    BehaviorFactory::RegisterBuiltins();
    RTSGameState state(Vector2(800.0f, 600.0f), 7);
    state.SetDiagnosticsEnabled(true);

    auto u1 = std::make_unique<RTSUnit>(Vector2(150.0f, 200.0f), 1, 14.0f, 120.0f);
    u1->GetStateMachine().AddState("Seek", BehaviorFactory::Create("Seek"));
    u1->GetStateMachine().SetState("Seek");
    u1->SetSelected(true);
    RTSUnit* rawU1 = u1.get();
    state.AddUnit(std::move(u1));

    auto u2 = std::make_unique<RTSUnit>(Vector2(400.0f, 200.0f), 2, 14.0f, 120.0f);
    state.AddUnit(std::move(u2));

    UERenderer renderer;
    renderer.SetGameState(&state);
    EXPECT_EQ(renderer.GetCurrentGameState(), &state);
    EXPECT_EQ(renderer.GetSimulationSnapshot(), &state);

    state.AddExplosion(Explosion(Vector2(140.0f, 200.0f), 100.0f, 2500.0f));
    renderer.NotePendingExplosions();
    state.Update(1.0f / 30.0f);
    renderer.Render(1.0f / 60.0f);

    const UE3DRenderFrame& frame = renderer.GetLastRenderFrame();
    EXPECT_EQ(frame.simulationTick, 1u);
    ASSERT_EQ(frame.units.size(), 2u);
    EXPECT_EQ(frame.units[0].entityId, state.GetEntityId(rawU1));
    EXPECT_EQ(frame.units[0].activeBehaviorState, "Seek");
    EXPECT_TRUE(frame.units[0].isSelected);
    EXPECT_FALSE(frame.explosions.empty());
    EXPECT_GT(frame.primitives.size(), 4u);

    // Aging past 0.5s clears explosion echoes
    renderer.Render(0.6f);
    EXPECT_TRUE(renderer.GetLastRenderFrame().explosions.empty());
}

TEST(UE5SupportTest, GenericInventoryEquipmentCartridgesAndSnapshotExtension) {
    BehaviorFactory::RegisterBuiltins();
    RTSGameState state(Vector2(800.0f, 600.0f), 11);
    auto unit = std::make_unique<RTSUnit>(Vector2(200.0f, 200.0f), 1, 12.0f, 100.0f);
    unit->SetHP(50.0f);
    RTSUnit* rawUnit = unit.get();
    state.AddUnit(std::move(unit));

    UEGenericInventory inv(8, 50.0f);
    EXPECT_EQ(inv.AddItemById("weapon_plasma_blade", 1), 1);
    EXPECT_EQ(inv.AddItemById("consumable_nanite_repair", 3), 3);
    EXPECT_EQ(inv.AddItemById("cartridge_flee", 1), 1);

    // Equip weapon
    ASSERT_TRUE(inv.EquipByItemId("weapon_plasma_blade", rawUnit));
    const UEItemStatModifiers mods = inv.ComputeTotalEquipmentModifiers();
    EXPECT_GT(mods.baseDpsDelta, 0.0f);
    EXPECT_GT(mods.attackRangeDelta, 0.0f);

    // Equip behavior cartridge ("Magic Dust") -> immediately switches unit FSM state to "Flee"
    ASSERT_TRUE(inv.EquipByItemId("cartridge_flee", rawUnit));
    EXPECT_EQ(rawUnit->GetStateMachine().GetCurrentState(), "Flee");

    // Use repair kit consumable -> heals HP, starts timed action, decrements stack
    uint64_t timedActionId = 0;
    ASSERT_TRUE(inv.UseItemOnUnit("consumable_nanite_repair", rawUnit, &state, nullptr, &timedActionId));
    EXPECT_FLOAT_EQ(rawUnit->GetCurrentHP(), 95.0f);
    EXPECT_NE(timedActionId, 0u);
    EXPECT_EQ(inv.GetItemCount("consumable_nanite_repair"), 2);

    // Snapshot capture, mutate, and restore
    const std::vector<uint8_t> snap = inv.CaptureSnapshotBytes();
    ASSERT_TRUE(inv.ValidateSnapshotBytes(snap));
    inv.Clear();
    EXPECT_EQ(inv.GetUsedSlotCount(), 0);
    ASSERT_TRUE(inv.RestoreSnapshotBytes(snap));
    EXPECT_EQ(inv.GetItemCount("consumable_nanite_repair"), 2);
    EXPECT_NE(inv.GetEquippedItem(UEEquipmentSlot::MainHand), nullptr);
}

TEST(UE5SupportTest, GenericCharacterStatsProjectilesAndTurnClash) {
    BehaviorFactory::RegisterBuiltins();
    RTSGameState state(Vector2(800.0f, 600.0f), 21);

    auto u1 = std::make_unique<RTSUnit>(Vector2(200.0f, 300.0f), 1, 14.0f, 100.0f);
    RTSUnit* rawU1 = u1.get();
    state.AddUnit(std::move(u1));

    auto u2 = std::make_unique<RTSUnit>(Vector2(350.0f, 300.0f), 2, 14.0f, 100.0f);
    u2->SetVelocity(Vector2(0.0f, 40.0f));
    RTSUnit* rawU2 = u2.get();
    state.AddUnit(std::move(u2));

    UEGenericCharacter hero1("Hero1", "Vanguard", 1);
    hero1.BindToUnit(state, rawU1);

    UEGenericCharacter hero2("Hero2", "Duelist", 2);
    hero2.BindToUnit(state, rawU2);

    const float initialMaxHp = rawU1->GetMaxHP();
    hero1.GetInventory().AddItemById("armor_aegis_plate", 1);
    ASSERT_TRUE(hero1.EquipItemAndRefresh("armor_aegis_plate", state));
    EXPECT_GT(rawU1->GetMaxHP(), initialMaxHp);

    // Level up via XP
    ASSERT_TRUE(hero1.AddExperience(150, &state));
    EXPECT_EQ(hero1.GetLevel(), 2);

    // Fire intercept projectile at moving target u2
    const uint64_t projId = hero1.FireInterceptProjectileAtUnit(state, rawU2, 300.0f, 20.0f);
    EXPECT_NE(projId, 0u);
    EXPECT_EQ(hero1.GetAnimState(), UECharacterAnimState::Attacking);

    // Execute TurnClashResolver clash between hero1 and hero2
    TelegraphResult telegraph;
    const ClashResolutionResult clash = hero1.ExecuteTurnClashWith(hero2, 0, 1, state, &telegraph);
    EXPECT_GE(clash.damageToInitiator + clash.damageToResponder, 0.0f);
    EXPECT_LT(rawU1->GetCurrentHP() + rawU2->GetCurrentHP(),
              rawU1->GetMaxHP() + rawU2->GetMaxHP());
}

TEST(UE5SupportTest, MinimapSystemProjectionsBlipsPingsAndClickCommands) {
    BehaviorFactory::RegisterBuiltins();
    RTSGameState state(Vector2(800.0f, 600.0f), 5);
    auto u1 = std::make_unique<RTSUnit>(Vector2(400.0f, 300.0f), 1, 14.0f, 100.0f);
    u1->SetSelected(true);
    RTSUnit* rawU1 = u1.get();
    state.AddUnit(std::move(u1));

    TerritoryCalculator territory;
    UECoordinateBridge bridge;
    UEMinimapSystem minimap;
    minimap.MarkHeroEntity(state.GetEntityId(rawU1), true);

    const uint64_t pingId = minimap.AddPing(Vector2(200.0f, 150.0f), state.GetMapSize(), 1, "Objective", 2.0f);
    EXPECT_NE(pingId, 0u);

    const UEMinimapFrame& frame = minimap.UpdateFrame(state, &territory, bridge, 0.1f);
    ASSERT_EQ(frame.unitBlips.size(), 1u);
    EXPECT_NEAR(frame.unitBlips[0].uv.x, 0.5f, 1e-4f);
    EXPECT_NEAR(frame.unitBlips[0].uv.y, 0.5f, 1e-4f);
    EXPECT_TRUE(frame.unitBlips[0].isHeroCharacter);
    ASSERT_EQ(frame.activePings.size(), 1u);

    // Left-click pans camera
    const Vector2 newCam = minimap.HandleLeftClickUV(Vector2(0.25f, 0.75f), state.GetMapSize());
    EXPECT_FLOAT_EQ(newCam.x, 200.0f);
    EXPECT_FLOAT_EQ(newCam.y, 450.0f);

    // Right-click orders selected unit to move
    ASSERT_TRUE(minimap.HandleRightClickUV(Vector2(0.75f, 0.5f), state));
    EXPECT_TRUE(rawU1->IsInDebugMode());
}

TEST(UE5SupportTest, Placeholder3DSandboxSingleAndMultiUnitBehaviorsAndStateMachines) {
    UEPlaceholder3DSandbox sandbox(Vector2(800.0f, 600.0f), 42);

    // 1. Single-Unit Behavior Lab: test individual behaviors
    sandbox.LoadPreset(UESandboxPreset::SingleUnitBehaviorLab);
    RTSUnit* hero = sandbox.GetPrimarySelectedOrFocusUnit();
    ASSERT_NE(hero, nullptr);
    EXPECT_EQ(hero->GetStateMachine().GetCurrentState(), "Seek");
    const Vector2 startPos = hero->GetPosition();

    for (int i = 0; i < 10; ++i) {
        sandbox.StepSingleTick();
    }
    EXPECT_GT((hero->GetPosition() - startPos).Length(), 1.0f);

    // Switch individual behavior to Flee and tune parameter
    ASSERT_TRUE(sandbox.AssignIndividualBehavior(0, "Flee"));
    EXPECT_EQ(hero->GetStateMachine().GetCurrentState(), "Flee");

    // Evaluate DecisionGraph when healthy (hp_pct = 1.0 >= 0.40 -> Action 2 = Seek)
    const DecisionEvalResult healthyDecision = sandbox.EvaluateDecisionGraphForUnit(0, true);
    EXPECT_EQ(healthyDecision.primaryActionId, 2);
    EXPECT_EQ(hero->GetStateMachine().GetCurrentState(), "Seek");

    // Damage unit below 40% HP and re-evaluate DecisionGraph (-> Action 1 = Flee)
    hero->SetHP(25.0f);
    const DecisionEvalResult lowHpDecision = sandbox.EvaluateDecisionGraphForUnit(0, true);
    EXPECT_EQ(lowHpDecision.primaryActionId, 1);
    EXPECT_EQ(hero->GetStateMachine().GetCurrentState(), "Flee");

    // 2. Single-Unit State Machine Lab: test automatic condition transitions
    sandbox.LoadPreset(UESandboxPreset::SingleUnitStateMachineLab);
    RTSUnit* fsmUnit = sandbox.GetPrimarySelectedOrFocusUnit();
    ASSERT_NE(fsmUnit, nullptr);
    EXPECT_EQ(fsmUnit->GetStateMachine().GetCurrentState(), "Wander");

    // On first tick, EnemyInRangeCondition(180) triggers Wander -> Seek automatically
    sandbox.StepSingleTick();
    EXPECT_EQ(fsmUnit->GetStateMachine().GetCurrentState(), "Seek");

    // After ~1.5s (46 ticks at 30Hz), TimeElapsedCondition(1.5f) triggers Seek -> Flee
    for (int i = 0; i < 48; ++i) {
        sandbox.StepSingleTick();
    }
    EXPECT_EQ(fsmUnit->GetStateMachine().GetCurrentState(), "Flee");

    // 3. Multi-Unit Squad & Swarm Lab: test N units per team + tactical checkers
    sandbox.LoadPreset(UESandboxPreset::MultiUnitSquadAndSwarm, 4);
    EXPECT_EQ(sandbox.GetGameState().GetAllUnits().size(), 8u);
    sandbox.StepSingleTick();
    EXPECT_GE(sandbox.GetTacticalAnalysisSummary().totalClusters, 2);

    // 4. Canonical snapshot save & restore (including character inventory game extension)
    ASSERT_TRUE(sandbox.SaveCanonicalSnapshot("slot1"));
    const uint64_t savedHash = sandbox.GetGameState().CanonicalStateHash();
    sandbox.StepSingleTick();
    sandbox.StepSingleTick();
    ASSERT_TRUE(sandbox.RestoreCanonicalSnapshot("slot1"));
    EXPECT_EQ(sandbox.GetGameState().CanonicalStateHash(), savedHash);

    // 5. Multiplayer Session OfflineResolution and OnlineSync lockstep demos
    const Session::MatchOutcome offlineOut = sandbox.RunOfflineSessionDemo(77);
    EXPECT_TRUE(offlineOut.complete);
    EXPECT_NE(offlineOut.stateHash, 0u);

    ASSERT_TRUE(sandbox.StartOnlineLockstepDemo(100, 99));
    sandbox.StepOnlineLockstepDemo(5, true);
    EXPECT_GE(sandbox.GetSessionDemoStatus().confirmedTick, 1u);
}

TEST(UE5SupportTest, DefaultGUIAndWorldSubsystemControllerEndToEnd) {
    FCoreEngineUE5Module pluginModule;
    pluginModule.StartupModule();

    UEWorldSubsystemController subsystem(Vector2(800.0f, 600.0f), 42);
    subsystem.Initialize(UESandboxPreset::SingleUnitBehaviorLab, 3);

    UEDefaultGUI& gui = subsystem.GetGUI();
    const UEGuiFrameModel& model = gui.GetLastFrameModel();
    EXPECT_EQ(model.panels.size(), 7u);

    // Execute GUI button actions across all subsystems
    EXPECT_TRUE(gui.ExecuteButtonAction("ai.set_behavior:TacticalFrontline"));
    EXPECT_EQ(subsystem.GetSandbox().GetPrimarySelectedOrFocusUnit()->GetStateMachine().GetCurrentState(),
              "TacticalFrontline");

    EXPECT_TRUE(gui.ExecuteButtonAction("deploy.grant_team1"));
    EXPECT_TRUE(gui.ExecuteButtonAction("unit.spawn_team1"));
    EXPECT_TRUE(gui.ExecuteButtonAction("combat.fire_projectile"));
    EXPECT_TRUE(gui.ExecuteButtonAction("combat.trigger_explosion"));
    EXPECT_TRUE(gui.ExecuteButtonAction("map.claim_resources"));
    EXPECT_TRUE(gui.ExecuteButtonAction("minimap.add_ping"));
    EXPECT_TRUE(gui.ExecuteButtonAction("snap.save_canonical"));

    // Slider adjustments
    EXPECT_TRUE(gui.ApplySliderValue("unit.max_speed", 175.0f));
    EXPECT_FLOAT_EQ(subsystem.GetSandbox().GetPrimarySelectedOrFocusUnit()->GetMaxSpeed(), 175.0f);

    // Tick the world subsystem controller
    subsystem.Tick(1.0f / 30.0f);
    EXPECT_GE(subsystem.GetSandbox().GetGameState().GetSimulationTick(), 1u);

    pluginModule.ShutdownModule();
}
