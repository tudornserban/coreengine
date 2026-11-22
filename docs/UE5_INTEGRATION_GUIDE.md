# CoreEngine — Unreal Engine 5 Integration Guide

This guide explains how to integrate **CoreEngine** into **Unreal Engine 5 (UE5)** with minimal overhead, how to use the built-in **3D Placeholder Sandbox**, **Generic Character**, **Generic Inventory**, **Minimap**, and **Default GUI**, and how to adopt CoreEngine in both **new** and **existing** UE5 game projects.

---

## 1. Architecture Overview

CoreEngine's UE5 support lives under [`platform/unreal/`](../platform/unreal/) and is designed around three principles:

1. **Zero FFI / Marshaling Overhead**: Both CoreEngine and UE5 are C++17. The UE5 plugin compiles `core/` and `platform/unreal/` directly in-process—no DLL P/Invoke copies or JSON serialization needed on the hot path.
2. **Strict Layer Separation**: `core/` remains 100% free of Unreal headers (`CoreMinimal.h`, `UObject`, etc.). All coordinate conversion, 3D placeholder rendering, character/inventory/minimap models, and GUI view-models live in `platform/unreal/`.
3. **Zero-Asset Out-of-the-Box Testing**: The default 3D placeholder renderer (`UERenderer`) and default GUI (`UEDefaultGUI`) work immediately using procedural/debug 3D shapes (capsules, orientation nose cones, 3D force arrows, wireframe spheres, rings, floating 3D badges) and Canvas/Slate/UMG view-models without requiring binary `.uasset` files.

### Components in `platform/unreal/`

| Component | Header / Implementation | Responsibility |
|-----------|------------------------|----------------|
| **Coordinate Bridge** | [`UECoordinateBridge.h`](../platform/unreal/UECoordinateBridge.h) | Maps CoreEngine 2D/3D coordinates (`Vector2`, `Vector3`) to UE5 3D world coordinates (`UEVec3` / `FVector` in cm), terrain height callbacks (`heightQuery`), heading yaw degrees, and 3D camera ray-to-ground intersection (`RaycastGroundToCore`). |
| **Input Adapter** | [`UEInputReader.h`](../platform/unreal/UEInputReader.h) | Implements [`IInputReader`](../core/interfaces/IInputReader.h), tracks frame-edge key/mouse states, projects 3D camera rays onto the arena floor, and drives [`SelectionSystem`](../core/combat/SelectionSystem.h) (click, Shift+click, 3D ground box drag) and [`CommandSystem`](../core/combat/CommandSystem.h) (right-click move). |
| **3D Placeholder Renderer** | [`UERenderer.h`](../platform/unreal/UERenderer.h) / [`.cpp`](../platform/unreal/UERenderer.cpp) | Implements [`IGameLogicRenderer`](../core/interfaces/IGameLogicRenderer.h). Produces a complete 3D scene snapshot (`UE3DRenderFrame`) each frame: unit capsules scaled by `GetVisualRadius()`, directional nose cones, selection rings, 3D velocity/propulsion/impulse arrows, perception/influence/attack radii, projectiles, 0.5s explosion shockwave echoes, obstacles, static attractors, territory 3D grid cells, and floating FSM state transition badges. |
| **Generic Inventory** | [`UEGenericInventory.h`](../platform/unreal/UEGenericInventory.h) / [`.cpp`](../platform/unreal/UEGenericInventory.cpp) | Slot-based, weight-limited inventory and equipment manager (`Weapon`, `Armor`, `Accessory`, `BehaviorCartridge` / Magic Dust, `Consumable`). Computes cumulative stat modifiers on `RTSUnit`, executes consumables/cartridges (`BehaviorFactory` state swap, healing, `DeploymentTimer::GrantCharge`, `TimedActionClock::Start`), and serializes to/from bytes for `RTSGameState::SetGameExtension`. |
| **Generic Character** | [`UEGenericCharacter.h`](../platform/unreal/UEGenericCharacter.h) / [`.cpp`](../platform/unreal/UEGenericCharacter.cpp) | Binds a 3D UE5 character/actor to a CoreEngine `RTSUnit` by `entityId`. Manages base stats + equipment recalculation, XP/leveling, stamina, 3D animation state (`Idle`, `Moving`, `Attacking`, `CastingTimedAction`, `HitImpulse`, `Dead`), lead-predicted projectile firing (`ProjectileSimulation::Intercept`), and [`TurnClashResolver`](../core/combat/TurnClashResolver.h) 5-Way Pentagon / 5-Precept Wheel clashes. |
| **Minimap & Radar** | [`UEMinimapSystem.h`](../platform/unreal/UEMinimapSystem.h) / [`.cpp`](../platform/unreal/UEMinimapSystem.cpp) | Projects world state into normalized minimap UVs and screen pixels: [`TerritoryCalculator`](../core/influence/TerritoryCalculator.h) heatmap cells, unit/hero/obstacle/resource/projectile blips, 3D camera frustum box, tactical radar pings (`AddPing`), left-click camera jump, and right-click minimap move orders. |
| **3D Placeholder Sandbox** | [`UEPlaceholder3DSandbox.h`](../platform/unreal/UEPlaceholder3DSandbox.h) / [`.cpp`](../platform/unreal/UEPlaceholder3DSandbox.cpp) | Interactive 3D testing harness for single-unit and multi-unit AI behaviors, generic `StateMachine` presets & custom transitions, [`DecisionGraph`](../core/ai/DecisionGraph.h) evaluation, combat, territory, canonical snapshots, and [`Session::InProcessHost`](../core/session/InProcessHost.h) offline & online lockstep matches. |
| **Default GUI** | [`UEDefaultGUI.h`](../platform/unreal/UEDefaultGUI.h) / [`.cpp`](../platform/unreal/UEDefaultGUI.cpp) | 7-panel default GUI view-model and interaction controller exposing every CoreEngine subsystem. |
| **UE5 Plugin Package** | [`CoreEngineUE5.uplugin`](../platform/unreal/CoreEngineUE5/CoreEngineUE5.uplugin), [`CoreEngineUE5.Build.cs`](../platform/unreal/CoreEngineUE5/Source/CoreEngineUE5/CoreEngineUE5.Build.cs), [`CoreEngineUE5Actors.h`](../platform/unreal/CoreEngineUE5/Source/CoreEngineUE5/Public/CoreEngineUE5Actors.h), [`CoreEngineVisualTestActor.h`](../platform/unreal/CoreEngineUE5/Source/CoreEngineUE5/Public/CoreEngineVisualTestActor.h) | Drop-in UE5 plugin module, `UEWorldSubsystemController`, `DrawUE5Placeholder3DFrame`, and turnkey `ACoreEngineVisualTestGameMode`, `ACoreEngineVisualTestActor`, and `ACoreEngineVisualTestHUD`. |

---

## 2. Using CoreEngine in a NEW UE5 Game Project

### Step 1: Install the `CoreEngineUE5` Plugin (One Command)

Run [`./scripts/install-ue5-plugin.sh`](../scripts/install-ue5-plugin.sh) pointing at your UE5 project directory:

```bash
./scripts/install-ue5-plugin.sh /path/to/MyUE5Game
```

This script copies `CoreEngineUE5`, bundles `core/` and `platform/unreal/` under `Plugins/CoreEngineUE5/ThirdParty/CoreEngine/`, and generates UnrealBuildTool translation unit wrappers under `Plugins/CoreEngineUE5/Source/CoreEngineUE5/Private/GeneratedCoreSources/` so Unreal Engine compiles all 69 C++17 translation units automatically when you open the project.

If you also want to reference CoreEngine C++ headers directly from your game module's C++ code, add `"CoreEngineUE5"` to `PublicDependencyModuleNames` in `MyUE5Game.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore", "CoreEngineUE5"
});
```

> **Note:** `CoreEngineUE5.Build.cs` automatically enables `CppStandardVersion.Cpp17`, `bUseUnity = false`, and `bUseRTTI = true` (required by CoreEngine's `dynamic_cast` interface gates such as `IUnitKinematics` and `IUnitAgent`).

---

### Step 2: Launch the Default 3D Placeholder Sandbox & GUI in Unreal Editor

Once `CoreEngineUE5` is installed in your UE5 project, you can run the visual test immediately with **zero** C++ or Blueprint code:

1. Open your `.uproject` in **Unreal Engine 5** (click **Yes** if prompted to build `CoreEngineUE5`).
2. Open **Window → World Settings** and set **GameMode Override** to **`CoreEngineVisualTestGameMode`** (or drag **`CoreEngineVisualTestActor`** from the *Place Actors* panel into your level).
3. Press **Play (Alt+P / Cmd+P)**:
   - **3D Viewport**: `ACoreEngineVisualTestActor` renders 3D unit capsules, orientation nose cones, selection rings, 3D velocity/propulsion/impulse arrows, perception/influence/attack spheres, projectiles, explosion shockwave rings, 3D territory grid quads, and floating 3D FSM state & HP labels.
   - **Interactive On-Screen GUI & Minimap**: `ACoreEngineVisualTestHUD` draws all 7 `UEDefaultGUI` panels with clickable buttons on the left/top and the 2D **Tactical Minimap & Radar** in the bottom-right corner.
   - **Mouse & Keyboard**: Left-click GUI buttons to trigger actions; left-click or drag-box in the 3D world to select units; right-click on the 3D floor (or on the Minimap) to order selected units to move; press `Space`, `.`, `1`–`4`, `R`, and `F1`–`F6` for playback and 3D overlay control.
   - **Details Panel Buttons**: Select `CoreEngineVisualTestActor` in the Outliner to click one-click `CallInEditor` buttons (`LoadSingleUnitBehaviorLab`, `LoadSingleUnitStateMachineLab`, `LoadMultiUnitSwarmLab`, `SwapBehaviorToSeek`, `FireTestProjectile`, `DetonateTestExplosion`, `TriggerTurnClash`, etc.).

Or, if you want to embed `CoreEngine::Unreal::UEWorldSubsystemController` inside your own custom `AActor` or `AGameModeBase`:

```cpp
#include "CoreEngineUE5Actors.h"

// Inside your AGameModeBase or Manager AActor:
// Header member:
//   std::unique_ptr<CoreEngine::Unreal::UEWorldSubsystemController> CoreSubsystem;

void AMySandboxActor::BeginPlay()
{
    Super::BeginPlay();

    CoreEngine::Unreal::UECoordinateConfig CoordCfg;
    CoordCfg.worldScale = 10.0f; // 1 CoreEngine unit = 10 cm in UE5 (800x600 -> 80m x 60m)
    CoordCfg.originOffset = CoreEngine::Unreal::UEVec3(0.0f, 0.0f, 0.0f);

    CoreSubsystem = std::make_unique<CoreEngine::Unreal::UEWorldSubsystemController>(
        Vector2(800.0f, 600.0f), /*seed=*/42, CoordCfg);

    // Start in the Single-Unit Behavior Lab (or any UESandboxPreset)
    CoreSubsystem->Initialize(
        CoreEngine::Unreal::UESandboxPreset::SingleUnitBehaviorLab, /*unitsPerTeam=*/4);
}

void AMySandboxActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!CoreSubsystem) return;

    CoreSubsystem->Tick(DeltaSeconds);

    // Render all 3D placeholder capsules, orientation cones, velocity/propulsion/impulse arrows,
    // perception/influence/attack spheres, projectiles, explosions, and floating FSM state badges:
    const auto& Frame3D = CoreSubsystem->GetSandbox().GetRenderer().GetLastRenderFrame();
    CoreEngine::Unreal::DrawUE5Placeholder3DFrame(GetWorld(), Frame3D, /*LifeTime=*/0.0f);
}
```

---

### Step 3: Testing Individual Behaviors and Generic State Machines (1 and N Units)

The 3D Sandbox (`UEPlaceholder3DSandbox`) includes 5 ready-made scenarios and a full programmatic API for testing individual behaviors and state machines:

#### A. Testing Individual Behaviors (1 or More Units)

```cpp
auto& Sandbox = CoreSubsystem->GetSandbox();

// 1. Register a custom game-specific behavior in BehaviorFactory (optional)
BehaviorFactory::Register("Aggro-Dust", [] {
    return std::make_unique<SeekBehavior>();
});

// 2. Load the Single-Unit Behavior Lab (1 controllable hero unit + 1 target + obstacle + attractor)
Sandbox.LoadPreset(CoreEngine::Unreal::UESandboxPreset::SingleUnitBehaviorLab);

// 3. Swap the active unit's behavior at runtime ("Magic Dust Swap")
Sandbox.AssignIndividualBehavior(/*entityId=0 uses selected/focus unit*/ 0, "TacticalFrontline");

// 4. Tune live behavior parameters on the unit
Sandbox.SetUnitBehaviorParameter(0, "weight", 1.5f);

// 5. Eject / clear the active behavior so the unit coasts without AI propulsion
Sandbox.EjectUnitBehaviorState(0);
```

#### B. Testing Generic State Machines (Automatic & Manual Transitions)

You can load a built-in multi-state FSM preset (`"PatrolEngageFlee"`, `"SwarmToFrontline"`, `"TimedBehaviorCycle"`) or assemble custom state machines dynamically:

```cpp
auto& Sandbox = CoreSubsystem->GetSandbox();

// Load Single-Unit State Machine Lab
Sandbox.LoadPreset(CoreEngine::Unreal::UESandboxPreset::SingleUnitStateMachineLab);
uint64_t UnitId = Sandbox.GetPrimaryFocusEntityId();

// Add states from BehaviorFactory
Sandbox.AddCustomFSMState(UnitId, "Patrol", "Wander");
Sandbox.AddCustomFSMState(UnitId, "Engage", "Seek");
Sandbox.AddCustomFSMState(UnitId, "Retreat", "Flee");

// Wire automatic condition transitions:
// 1) Patrol -> Engage when an enemy enters 180 CoreEngine units
Sandbox.AddEnemyInRangeTransition(UnitId, "Patrol", "Engage", 180.0f);

// 2) Engage -> Retreat after 2.0 seconds of engagement
Sandbox.AddTimeElapsedTransition(UnitId, "Engage", "Retreat", 2.0f);

// 3) Retreat -> Patrol after 1.5 seconds of fleeing
Sandbox.AddTimeElapsedTransition(UnitId, "Retreat", "Patrol", 1.5f);

// Start in Patrol
Sandbox.TriggerManualStateSwap(UnitId, "Patrol");
```

When an automatic transition fires during `RTSGameState::Update`, `UERenderer` highlights the unit's 3D floating badge in gold and displays `(<fromState>-><toState>)` above the unit in 3D space.

#### C. Testing Multi-Unit Squads, Swarms, and Tactical Checkers

```cpp
// Spawn 6 units per team configured with Cohesion -> TacticalFrontline -> Separation FSMs
Sandbox.LoadPreset(CoreEngine::Unreal::UESandboxPreset::MultiUnitSquadAndSwarm, /*unitsPerTeam=*/6);

// Inspect live tactical metrics computed each tick by ClusterDetector, EncirclementChecker, and SurroundChecker:
const auto& Tactical = Sandbox.GetTacticalAnalysisSummary();
// Tactical.totalClusters, Tactical.team1Clusters, Tactical.team2Clusters,
// Tactical.encircledClusters, Tactical.surroundedEntityIds
```

---

### Step 4: Using Generic Character, Inventory, and Minimap in a New Game

#### Generic Character & Inventory

Every unit spawned with `createCharacterWrapper = true` has a bound [`UEGenericCharacter`](../platform/unreal/UEGenericCharacter.h) and [`UEGenericInventory`](../platform/unreal/UEGenericInventory.h):

```cpp
uint64_t HeroId = Sandbox.SpawnTestUnit(/*teamId=*/1, Vector2(250.0f, 300.0f), "Seek", /*createCharacterWrapper=*/true);
auto* Hero = Sandbox.FindCharacterByEntityId(HeroId);

// Add items from the catalog (or register custom UEItemDefinition entries)
Hero->GetInventory().AddItemById("weapon_plasma_blade", 1);
Hero->GetInventory().AddItemById("armor_aegis_plate", 1);
Hero->GetInventory().AddItemById("consumable_nanite_repair", 3);
Hero->GetInventory().AddItemById("cartridge_frontline", 1);

// Equip weapon & armor -> automatically recomputes RTSUnit MaxHP, MaxSpeed, BaseDPS, AttackRange, MassModifier
Hero->EquipItemAndRefresh("weapon_plasma_blade", Sandbox.GetGameState());
Hero->EquipItemAndRefresh("armor_aegis_plate", Sandbox.GetGameState());

// Equip a Behavior Cartridge ("Magic Dust") -> swaps the unit's StateMachine state to "TacticalFrontline"
Hero->EquipItemAndRefresh("cartridge_frontline", Sandbox.GetGameState());

// Use a repair kit -> heals HP, starts a 15-tick TimedActionClock calibration, and triggers slot cooldown
Hero->UseInventoryItem("consumable_nanite_repair", Sandbox.GetGameState(), &Sandbox.GetDeploymentTimer(1));

// Fire an intercept-predicted projectile at a moving enemy unit
Hero->FireInterceptProjectileAtUnit(Sandbox.GetGameState(), EnemyUnitPtr, /*speed=*/280.0f, /*damage=*/25.0f);
```

#### Minimap & Tactical Radar

[`UEMinimapSystem`](../platform/unreal/UEMinimapSystem.h) produces a complete `UEMinimapFrame` every tick:

```cpp
auto& Minimap = Sandbox.GetMinimapSystem();

// Trigger a 3-second pulsing radar ping at world position (400, 300)
Minimap.AddPing(Vector2(400.0f, 300.0f), Sandbox.GetGameState().GetMapSize(), /*team=*/1, "DefendHere", 3.0f);

// Convert screen click (px, py) to minimap UV and issue a move command to selected units:
Vector2 ClickedUV;
if (Minimap.ScreenPixelsToMinimapUV(Vector2(MouseX, MouseY), &ClickedUV)) {
    if (bRightMouseButton) {
        Minimap.HandleRightClickUV(ClickedUV, Sandbox.GetGameState());
    } else if (bLeftMouseButton) {
        Vector2 NewCameraCenterCore = Minimap.HandleLeftClickUV(ClickedUV, Sandbox.GetGameState().GetMapSize());
    }
}
```

---

## 3. Integrating CoreEngine into an EXISTING UE5 Game Project

Existing UE5 games typically already have their own `ACharacter` / `APawn` subclasses, animation blueprints, Enhanced Input assets, and UMG HUD widgets. CoreEngine is designed to slot underneath your existing presentation layer as a deterministic simulation and AI/combat authority without forcing you to rewrite your UE5 actors.

### Integration Pattern A: Bind Existing `ACharacter` / `AActor` Classes to CoreEngine Units

Instead of replacing your `ACharacter` hierarchy, store the CoreEngine `uint64_t EntityId` on your existing actor or on a lightweight `UActorComponent`:

```cpp
// In your existing AMyExistingCharacter.h:
uint64_t CoreEntityId = 0;

// In AMyExistingCharacter::BeginPlay():
auto& Sandbox = GetCoreSubsystem()->GetSandbox();
const FVector MyLoc = GetActorLocation();
const Vector2 CoreStartPos = Sandbox.GetCoordinateBridge().UEToCorePosition(
    CoreEngine::Unreal::UEVec3(MyLoc.X, MyLoc.Y, MyLoc.Z));

CoreEntityId = Sandbox.SpawnTestUnit(TeamId, CoreStartPos, "TacticalFrontline", /*createCharacterWrapper=*/true);

// In AMyExistingCharacter::Tick(float DeltaSeconds):
if (const auto* CoreChar = Sandbox.FindCharacterByEntityId(CoreEntityId))
{
    const auto& Xf = CoreChar->GetTransform3D();
    SetActorLocation(FVector(Xf.location.x, Xf.location.y, GetActorLocation().Z));
    SetActorRotation(FRotator(0.0f, Xf.rotation.yaw, 0.0f));

    // Drive your existing AnimBlueprint state machine from CoreChar->GetAnimState():
    // Idle, Moving, Attacking, CastingTimedAction, HitImpulse, Dead
}
```

### Integration Pattern B: Connect UE5 Landscape / NavMesh Height to `UECoordinateBridge`

If your existing game has 3D terrain elevation, configure `UECoordinateConfig::heightQuery` once during level initialization so all CoreEngine 3D transforms, projectiles, and debug overlays snap to your UE5 terrain:

```cpp
CoreEngine::Unreal::UECoordinateConfig Config = Sandbox.GetCoordinateBridge().GetConfig();
UWorld* World = GetWorld();

Config.heightQuery = [World, WorldScale = Config.worldScale](float CoreX, float CoreY) -> float {
    const FVector TraceStart(CoreX * WorldScale, CoreY * WorldScale, 5000.0f);
    const FVector TraceEnd(CoreX * WorldScale, CoreY * WorldScale, -5000.0f);
    FHitResult Hit;
    if (World && World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic)) {
        return Hit.ImpactPoint.Z;
    }
    return 0.0f;
};
Sandbox.GetCoordinateBridge().SetConfig(Config);
```

### Integration Pattern C: Route UE5 Enhanced Input & Camera Raycasts into `UEInputReader`

In your existing `APlayerController::PlayerTick`:

```cpp
auto& Input = Sandbox.GetInputReader();
const auto& Bridge = Sandbox.GetCoordinateBridge();

FVector WorldOrigin, WorldDirection;
if (DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
{
    Input.InjectWorldCameraRay(
        CoreEngine::Unreal::UEVec3(WorldOrigin.X, WorldOrigin.Y, WorldOrigin.Z),
        CoreEngine::Unreal::UEVec3(WorldDirection.X, WorldDirection.Y, WorldDirection.Z),
        Bridge);
}

Input.InjectMouseButtonState(CoreEngine::Unreal::UE_MOUSE_LEFT, IsInputKeyDown(EKeys::LeftMouseButton));
Input.InjectMouseButtonState(CoreEngine::Unreal::UE_MOUSE_RIGHT, IsInputKeyDown(EKeys::RightMouseButton));
Input.InjectKeyState(CoreEngine::Unreal::UE_KEY_LSHIFT, IsInputKeyDown(EKeys::LeftShift));
```

### Integration Pattern D: Hook Existing Game State into Deterministic Rollback Snapshots

If your existing UE5 game has custom match counters, inventory state, or ability cooldowns outside `RTSUnit`, register them with [`RTSGameState::SetGameExtension`](../core/gamestate/RTSGameState.h) so [`CaptureCanonicalState`](../core/gamestate/RTSGameState.h) and [`RestoreCanonicalState`](../core/gamestate/RTSGameState.h) snapshot and rollback your game state atomically:

```cpp
RTSGameState& Sim = Sandbox.GetGameState();
Sim.SetGameExtension(
    // 1. Capture callback -> std::vector<uint8_t>
    [this]() { return MyExistingInventory.CaptureSnapshotBytes(); },
    // 2. Validate callback (must not mutate!) -> bool
    [this](const std::vector<uint8_t>& Bytes) { return MyExistingInventory.ValidateSnapshotBytes(Bytes); },
    // 3. Apply callback (called only after core snapshot validation succeeds)
    [this](const std::vector<uint8_t>& Bytes) { MyExistingInventory.RestoreSnapshotBytes(Bytes); }
);
```

### Integration Pattern E: Multiplayer Lockstep & Offline Resolution (`core.session`)

To use CoreEngine's multiplayer lobby and server-authoritative lockstep ([`core/session/`](../core/session/MODULE.md)) in an existing UE5 project:

1. Implement [`Session::IDeterministicMatch`](../core/session/IDeterministicMatch.h) and [`Session::IMatchFactory`](../core/session/IDeterministicMatch.h) wrapping your `RTSGameState` (see `UESandboxDeterministicMatch` in [`UEPlaceholder3DSandbox.cpp`](../platform/unreal/UEPlaceholder3DSandbox.cpp) for a complete working template).
2. For local/single-process testing or listen-server matches, use [`Session::InProcessHost`](../core/session/InProcessHost.h).
3. For dedicated server deployment over UE5 sockets (`FSocket` / EOS / SteamSockets), implement [`Session::ISessionTransport`](../core/session/ISessionTransport.h) to carry `Session::Envelope` byte packets between `SessionClient` and `SessionServer`.

---

## 4. Default GUI Panels & Keyboard Shortcuts Reference

[`UEDefaultGUI`](../platform/unreal/UEDefaultGUI.h) builds a 7-panel `UEGuiFrameModel` every frame and processes widget actions via `ExecuteButtonAction(widgetId)` and `ApplySliderValue(widgetId, value)`:

| Panel | CoreEngine Modules Covered | Key Controls & Telemetry |
|-------|---------------------------|--------------------------|
| **1. Simulation, Playback & Match Loop** | `core.gameloop`, `core.math` | Scenario preset buttons, Pause/Resume, Step +1 Tick (`1/30` s), Speed (`0.5x`, `1x`, `2x`, `4x`), Fast-Forward, Reset, Tick counter, `DeterministicRNG` seed, `DeploymentTimer` charges/progress & `+1 Charge` buttons, `ThinkTimer` tactical pause toggle, `MatchState` dominance & timer bars. |
| **2. AI Behaviors, State Machine & Decision Graph** | `core.ai` | All registered `BehaviorFactory` behaviors (one-click state swap), Eject/Clear state, generic `StateMachine` presets (`PatrolEngageFlee`, `SwarmToFrontline`, `TimedBehaviorCycle`), dynamic transition builders (`EnemyInRangeCondition`, `AlliesNearbyCondition`, `TimeElapsedCondition`), live transition timer states, live `IBehavior::GetParameters()` sliders, and `DecisionGraphInterpreter` evaluation. |
| **3. Unit, Character, Inventory & Timed Actions** | `core.gamestate`, Character, Inventory | Spawn Team 1 / Team 2 units, selected `RTSUnit` HP/Position/Velocity/Mass/Radius sliders, participation toggles (`Movement`, `Actions`, `Collision`), `TimedActionClock` start/cancel, `UEGenericCharacter` Level/XP/Stamina/3D AnimState, and `UEGenericInventory` Equip / Use / Magic Dust Cartridge buttons. |
| **4. CombatManager, Projectiles, Explosions & Turn Clash** | `core.combat` | `CombatManager` toggle, `CombatMode` (`InfluenceAttrition`, `DirectAttack`, `Hybrid`), `TargetPolicy` (`Nearest`, `LowestHP`, `HighestThreat`), fire lead-predicted `ProjectileSimulation` shot, detonate center `Explosion` impulse, and execute `TurnClashResolver` 5-Way Pentagon / 5-Precept Wheel clashes. |
| **5. 3D Overlays, Territory, Tactical Checkers & Map** | `core.influence`, `core.map`, `core.collision` | Toggle 3D arrows/spheres/rings/territory grid/state badges, `TerritoryCalculator` Voronoi percentages, `ClusterDetector` counts, `EncirclementChecker` liberty count, `SurroundChecker` surrounded unit count, spawn 3D obstacles/attractors, and `ResolveResourceClaims`. |
| **6. Canonical Snapshots, JSON & Multiplayer Session** | `core.serialization`, `core.session` | `CaptureCanonicalState` quicksave, `RestoreCanonicalState` quickload, `CanonicalStateHash`, `GameStateSerializer` diagnostic JSON, `InProcessHost` `OfflineResolution` and `OnlineSync` lockstep runner with RTT slider (`InputDelayTicksForRtt`, `InterpolationDelayMs`). |
| **7. Tactical Minimap & Radar** | `UEMinimapSystem`, `core.influence` | 2D radar view, territory heatmap toggle, unit/hero/obstacle/resource/projectile blips, 3D camera viewport box, `AddPing` button, left-click camera pan, right-click move order. |

### Keyboard Shortcuts (`UEDefaultGUI::ProcessKeyboardShortcuts`)

| Key | Action |
|-----|--------|
| `Space` | Toggle Pause / Resume |
| `.` (Period) | Step +1 fixed simulation tick (`1/30` s) |
| `1` / `2` / `3` / `4` | Set playback speed to `0.5x` / `1.0x` / `2.0x` / `4.0x` |
| `R` | Reset current 3D sandbox preset |
| `F1` | Toggle 3D Velocity Arrows |
| `F2` | Toggle 3D Propulsion & Explosion Impulse Arrows |
| `F3` | Toggle 3D Perception Radii |
| `F4` | Toggle 3D Influence Field Spheres & Attractor Beacons |
| `F5` | Toggle 3D Attack Range Rings |
| `F6` | Toggle 3D Territory Ownership Grid |
