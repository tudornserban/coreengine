# CoreEngine — Architecture Overview

**Version:** 1.0.0  
**Source:** Extracted and generalized from VenusWars, DynamicGO, AscendantExpanse, RationalInsanity

---

## Design Goals

1. **Platform-agnostic core**: zero SDL2/Raylib/UE5/Unity headers in `core/`
2. **Language-interoperable**: flat C API in `ffi/` for Python, Rust, Go, Lua, Unity C#
3. **Deterministic**: seeded RNG + fixed-timestep loop → reproducible replays on the same binary and platform ([DETERMINISM.md](DETERMINISM.md))
4. **Extensible**: behavior factory enables runtime behavior swapping (RationalInsanity)
5. **Future-portable**: compile `core/` unchanged into UE5, Unity, or any C++17 environment

---

## Layer Map

```
├── core/           ← LAYER 1: Zero platform deps, pure C++17 STL
│   ├── math/
│   ├── interfaces/
│   ├── ai/
│   ├── influence/
│   ├── collision/
│   ├── gameloop/
│   ├── map/
│   ├── combat/
│   ├── gamestate/
│   ├── logging/
│   ├── ai_advanced/
│   ├── serialization/
│   └── session/
├── platform/       ← LAYER 2: Platform adapter headers (implement core/ interfaces)
│   ├── host/       (HostPlatform.h — macOS / Windows / Android / Linux macros)
│   ├── sdl2/       (SDLRenderer / SDLInputReader — stubs; no .cpp in repo)
│   ├── raylib/     (RaylibRenderer::Render() unimplemented; header-only)
│   ├── null/
│   ├── unreal/
│   └── unity/
├── cmake/          ← Host toolchains (macos osxcross, windows MinGW, Android NDK)
├── ffi/            ← LAYER 3: C API for foreign language interop
│   ├── c_api.h
│   ├── c_api.cpp
│   ├── python/
│   └── lua/
├── tools/          ← Optional SDL2 viewer (COREENGINE_BUILD_SDL2_VIEWER)
├── scripts/        ← Module checks, standalone runners, doc link checker
├── tests/          ← GTest when BUILD_TESTS=ON
└── docs/
```

---

## Layer 1: `core/` — Platform-Agnostic Engine

**Rule:** No `#include <SDL.h>`, `#include <raylib.h>`, no UE headers. Pure STL only.

### `core/math/`
| File | Purpose |
|------|---------|
| `Vector2.h` | 2D vector with arithmetic, normalization, lerp, clamp |
| `Vector3.h` | 3D vector stub for AscendantExpanse 3D maps |
| `DeterministicRNG.h/cpp` | Seeded LCG — same seed = same sequence every time |
| `MathConstants.h` | PI, TWO_PI, DEG_TO_RAD, EPSILON |

### `core/interfaces/`
These are the FFI boundary contracts — the minimal surface area each system depends on.

| Interface | Purpose |
|-----------|---------|
| `IGameUnit.h` | Position, velocity, HP, team, radius |
| `IUnitKinematics.h` | Optional `SetPosition` / `SetVelocity` for systems that must write kinematics (e.g. collision); `RTSUnit` implements |
| `IBehavior.h` | GenerateFieldItems() or CalculateAcceleration() + lifecycle hooks |
| `ITransitionCondition.h` | Evaluate(BehaviorContext) → bool |
| `IGameState.h` | OnEnter/OnExit/Update/Render |
| `IGameLogicRenderer.h` | GetCurrentGameState() — read-only renderer contract; GetSimulationSnapshot() optional (default nullptr) |
| `IInputReader.h` | PollEvents(), IsKeyDown(), GetMousePosition() etc. |
| `ISerializer.h` | Serialize/Deserialize to byte buffer or string |
| `IMapComponent.h` | ECS component base for map entities |

### `core/ai/`
- `StateMachine` — generic FSM; behavior states + condition-driven transitions
- `BehaviorContext` — per-tick context struct (self, allies, enemies, RNG, map)
- `BehaviorFactory` — runtime behavior creation by string key (magic dust pattern)
- Behaviors: `Seek`, `Flee`, `Wander`, `Cohesion`, `Separation`, `TacticalFrontline`
- Conditions: `EnemyInRange`, `AlliesNearby`, `TimeElapsed`

### `core/influence/`
Merged from VenusWars PotentialField* and DynamicGO InfluenceMap.

- `InfluenceFieldItem` — attractor/repulsor/edge entry with typed parameters
- `InfluenceField::CalculateTotalForce` — APF total force from item list (clamped to maxAccel)
- `InfluenceFieldParameters` — AttractorParams, RepulsorParams, EdgeParams
- `TerritoryCalculator` — Voronoi grid ownership % (win condition system) plus N-team `UpdateGrid` (active / passive / contested / obstacle, timed capture)
- `ClusterDetector` / `EncirclementChecker` / `SurroundChecker` — proximity clusters, liberty flood-fill, and circumference surround checks

### `core/collision/`
Directly from VenusWars — broadphase SpatialGrid + narrowphase circle detection.

- `CollisionSystem` — orchestrates detection + resolution + events
- `CollisionDetector` / `NarrowPhaseDetector` / `SpatialGrid` — broadphase + narrowphase
- `CollisionResolver` / `PositionCorrector` / `MapCollisionHandler`
- `CollisionEventManager` / `ImmunityManager`
- Type headers: `CollisionContact`, `CollisionEvent`, `CollisionTypes`, `CollisionSettings`

### `core/gameloop/`
- `GameLogicEngine` — fixed 30Hz logic tick, decoupled from render Hz, pause/speed/fast-forward
- `PlaybackController` — thin wrapper for playback control UI
- `DeploymentTimer` — multi-charge spawn timer with optional initial fill and `GrantCharge()`
- `ThinkTimer` — tactical pause / think-time bank (from DynamicGO)
- `MatchState` — canonical 2-team match state with territory/time win conditions
- `MultiTeamMatchState` — same hold and time-limit rules for FFA / N teams

### `core/map/`
- `Map` — ECS-based arena (rectangle/circle/polygon shapes)
- `MapEntity` / `IMapComponent` — entity + component system for map objects
- `MapObstacleComponent` / `MapResourceComponent` — built-in component types. Obstacles emit the shared `Obstacle` disc.
- `PlacementValidator` — bounds, placement obstacles, team and enemy clearance, optional territory predicate, optional candidate radius
- `ResolveResourceClaims` — one grant per resource, lowest claimant id wins
- `Obstacle` — stateless disc (`blocksPlacement`, `blocksInfluence`) used by map, influence, and validators
- `GameMode` — DETERMINISTIC vs INTERACTIVE enum

### `core/combat/`
- `CombatManager` — simultaneous influence-attrition, direct-attack, and hybrid resolution on `IUnitCombat` / `IInfluenceEmitter`. Shot timers for units with an entity id can be exported and restored
- `ProjectileSimulation` — swept projectiles. Games choose firing policy, charge, and armor
- `Explosion` — area velocity change with falloff. The impulse is already mass-scaled
- `SelectionSystem` — click/box unit selection
- `CommandSystem` — unit state overrides, move-to commands, parameter tuning

### `core/gamestate/`
- `RTSGameState` — simulation hub. `CaptureCanonicalState` / `RestoreCanonicalState` is the restorable snapshot. Diagnostic JSON is a separate, non-restoring view
- `RTSUnit` — concrete unit with a state machine, split propulsion and impulses, and a game-supplied visual-radius curve
- `TimedActionClock` — start, progress, complete, cancel, and interrupt counted in ticks
- `GameStateManager` — single active state (transitions via SetState)
- `IGameState` / `IGameLogicRenderer` — thin `#include` forwarders to `core/interfaces/` (canonical contract)

### `core/logging/`
- `VenusWars::Logging::Logger` (singleton) — level-filtered console + file output, `LOG_DEBUG/INFO/WARN/ERROR/FATAL` macros

### `core/ai_advanced/`
- `Genome` — feature weight vector for placement evaluation
- `BotPlayer` — AI player using Genome + configurable PlacementEvaluator
- `GeneticTrainer` — genetic algorithm for evolving Genome populations

### `core/serialization/`
- `GameStateSerializer` — diagnostic JSON of `RTSGameState`. It does not restore a match. Rollback uses the canonical snapshot on `RTSGameState`

### `core/session/`
Lobby and client-server session. No sockets in `core/` — `ISessionTransport` is the byte pipe, and `InProcessHost` is the reference server plus clients in one process.

- `SessionServer` / `SessionClient` — create/join/ready lobby, matchmaking by `GameTitle` + `PlayMode`
- `PlayMode::OfflineResolution` — each client submits a `DecisionPlan`; `ResolveOffline` runs an `IDeterministicMatch` on the server and returns one `MatchOutcome` (same hash a client gets by replaying)
- `PlayMode::OnlineSync` — server-authoritative lockstep. Input delay tracks the worst RTT; late commands inside the rollback window restore a snapshot and resimulate; remote presentation waits half an RTT
- `GameTitle` — `DynamicGo`, `AscendantExpanse`, `ZodiacWars`, `VenusWars`. Each game supplies `IMatchFactory`; this module does not contain their rules
- `MatchSetup` — opaque content reference and payload stored on the lobby before start. `IDeterministicMatch::Configure` receives it. Persistent accounts and authenticated transport stay outside this module

The authoritative tick clock stays on the server (`AdvanceOnlineMatches`). Clients send commands; they do not advance time.

---

## Layer 2: `platform/` — Platform Adapters

Each adapter implements interfaces from `core/interfaces/`. The core engine never includes platform headers. **Today:** `platform/sdl2` and `platform/raylib` are **header stubs** only (`Render()` not implemented; no `.cpp` in-repo). `-DBUILD_SDL2=ON` or `-DBUILD_RAYLIB=ON` **fail at configure** with an explicit `header-only stub` message (before any `find_package` for those adapters). Headers remain the extension point for ports. For a windowed SDL2 tick without writing adapters, use **`COREENGINE_BUILD_SDL2_VIEWER`** (`tools/viewer/`, [VIEWER.md](VIEWER.md)).

**Host OS builds** (macOS, Windows, Android) are not rendering adapters. CMake records `COREENGINE_TARGET_PLATFORM` and `platform/host/HostPlatform.h` exposes the matching macros. Toolchains and the `./scripts/build-host-platform.sh` entry point are documented in [HOST_PLATFORMS.md](HOST_PLATFORMS.md). `core/` does not include `platform/host`.

| Adapter | Implements | Use |
|---------|-----------|-----|
| `sdl2/` | `IGameLogicRenderer`, `IInputReader` | Reference stubs; copy from VenusWars or use `tools/viewer` |
| `raylib/` | `IGameLogicRenderer`, `IInputReader` | Reference stubs; `RaylibRenderer::Render()` is declared but not implemented |
| `null/` | `IGameLogicRenderer`, `IInputReader` | Headless server, unit tests |
| `unreal/` | Stubs + porting guide | UE5 integration |
| `unity/` | Stubs → delegates to ffi/ | Unity C# P/Invoke |

---

## Layer 3: `ffi/` — C API

Flat C functions, no C++ types crossing the boundary.

```
ffi/
├── c_api.h        ← public header (include from any language)
├── c_api.cpp      ← implementation (links coreengine static lib)
├── python/        ← ctypes/cffi stubs + usage example
└── lua/           ← LuaJIT FFI bindings
```

Build: `cmake -DBUILD_FFI=ON ..` → `libcoreengine_ffi.so`. WebAssembly builds link the same `coreengine_ffi` target with Emscripten (see [FFI_GUIDE.md](FFI_GUIDE.md)).

---

## Fixed Timestep

Logic runs at 30 Hz (configurable). Render runs at any Hz.

```
each frame:
  logicEngine.Update(wallClockDelta)  // accumulates + fires fixed ticks
  renderer.Render()                   // reads RTSGameState snapshot (read-only)
```

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| C API as FFI boundary | Only safe cross-language boundary; no C++ ABI dependency |
| `BehaviorFactory` | Enables RationalInsanity magic dust swap + future Lua scripting |
| `DeterministicRNG` in core | Required for replay determinism in all games |
| `InfluenceFieldItem` (renamed from PotentialFieldItem) | Generic name fits all games |
| `TerritoryCalculator` in core | Shared win condition system for DynamicGO + AscendantExpanse |
| `DeploymentTimer` in core | Universal spawn charge system across all four games |
| `IGameLogicRenderer` read-only | Logic state never mutated by renderer |

---

## Optional viewer (`tools/viewer/`)

`core/` remains free of SDL, platform adapters, and FFI headers. The optional SDL2 simulation viewer (`-DCOREENGINE_BUILD_SDL2_VIEWER=ON`, default OFF) lives entirely under `tools/viewer/` and links the existing `coreengine` static library. It is not registered in `docs/agentic/modules/` and does not change engine simulation code. See [VIEWER.md](VIEWER.md).
