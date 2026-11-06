# CoreEngine — Architecture Overview

**Version:** 1.0.0  
**Source:** Extracted and generalized from VenusWars, DynamicGO, AscendantExpanse, RationalInsanity

---

## Design Goals

1. **Platform-agnostic core**: zero SDL2/Raylib/UE5/Unity headers in `core/`
2. **Language-interoperable**: flat C API in `ffi/` for Python, Rust, Go, Lua, Unity C#
3. **Deterministic**: seeded RNG + fixed-timestep loop → reproducible replays
4. **Extensible**: behavior factory enables runtime behavior swapping (RationalInsanity)
5. **Future-portable**: compile `core/` unchanged into UE5, Unity, or any C++17 environment

---

## Layer Map

```
CoreEngine/
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
│   └── serialization/
├── platform/       ← LAYER 2: Platform adapters (implement core/ interfaces)
│   ├── sdl2/
│   ├── raylib/
│   ├── null/
│   ├── unreal/
│   └── unity/
├── ffi/            ← LAYER 3: C API for foreign language interop
│   ├── c_api.h
│   ├── c_api.cpp
│   ├── python/
│   ├── lua/
│   └── wasm/
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
| `IGameLogicRenderer.h` | GetCurrentGameState() — read-only renderer contract |
| `IInputReader.h` | PollEvents(), IsKeyDown(), GetMousePosition() etc. |
| `ISerializer.h` | Serialize/Deserialize to byte buffer or string |
| `IMapComponent.h` | ECS component base for map entities |

### `core/ai/`
- `StateMachine` — generic FSM; behavior states + condition-driven transitions
- `BehaviorContext` — per-tick context struct (self, allies, enemies, RNG, map)
- `BehaviorFactory` — runtime behavior creation by string key (magic dust pattern)
- Behaviors: `Seek`, `Flee`, `Wander`, `Cohesion`, `Separation`
- Conditions: `EnemyInRange`, `AlliesNearby`, `TimeElapsed`

### `core/influence/`
Merged from VenusWars PotentialField* and DynamicGO InfluenceMap.

- `InfluenceFieldItem` — attractor/repulsor/edge entry with typed parameters
- `InfluenceFieldCalculator` — APF total force from item list (clamped to maxAccel)
- `InfluenceFieldParameters` — AttractorParams, RepulsorParams, EdgeParams
- `TerritoryCalculator` — Voronoi grid ownership % (win condition system)

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
- `DeploymentTimer` — charge-based spawn timer (single-charge strict; from DynamicGO)
- `ThinkTimer` — tactical pause / think-time bank (from DynamicGO)
- `MatchState` — canonical 2-team match state with territory/time win conditions

### `core/map/`
- `Map` — ECS-based arena (rectangle/circle/polygon shapes)
- `MapEntity` / `IMapComponent` — entity + component system for map objects
- `MapObstacleComponent` / `MapResourceComponent` — built-in component types
- `PlacementValidator` — validates unit spawn positions (no overlap, in-bounds)
- `GameMode` — DETERMINISTIC vs INTERACTIVE enum

### `core/combat/`
- `CombatManager` — attrition damage + zone overlap combat (from DynamicGO)
- `Explosion` — area impulse/damage with falloff
- `SelectionSystem` — click/box unit selection
- `CommandSystem` — unit state overrides, move-to commands, parameter tuning

### `core/gamestate/`
- `RTSGameState` — canonical simulation state (units, map, collision, RNG)
- `RTSUnit` — concrete IGameUnit with StateMachine + InfluenceFieldController
- `GameStateManager` — single-active-state stack (transitions via SetState)
- `IGameState` / `IGameLogicRenderer` — mirror interfaces (also in `core/interfaces/`)

### `core/logging/`
- `Logger` (singleton) — level-filtered console + file output, `LOG_DEBUG/INFO/WARN/ERROR/FATAL` macros

### `core/ai_advanced/`
- `Genome` — feature weight vector for placement evaluation
- `BotPlayer` — AI player using Genome + configurable PlacementEvaluator
- `GeneticTrainer` — genetic algorithm for evolving Genome populations

### `core/serialization/`
- `GameStateSerializer` — JSON snapshot of RTSGameState (for replay, FFI, server)

---

## Layer 2: `platform/` — Platform Adapters

Each adapter implements interfaces from `core/interfaces/`. The core engine never includes platform headers.

| Adapter | Implements | Use |
|---------|-----------|-----|
| `sdl2/` | `IGameLogicRenderer`, `IInputReader` | VenusWars, standalone prototypes |
| `raylib/` | `IGameLogicRenderer`, `IInputReader` | DynamicGO, lightweight prototypes |
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
├── lua/           ← LuaJIT FFI bindings
└── wasm/          ← WebAssembly stub (Emscripten)
```

Build: `cmake -DBUILD_FFI=ON ..` → `libcoreengine_ffi.so`

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
