# CoreEngine — Phase 0 Inventory

**Date:** 2026-11-11 (refreshed)  
**Repo layout:** `core/` (engine), `platform/` (adapter headers), `ffi/` (C API), `tools/viewer/` (optional SDL2 viewer), `scripts/`, `tests/`, `docs/`  
**Build:** static `coreengine` links **OBJECT** libraries `coreengine_<module>` from `core/*/CMakeLists.txt`, plus optional `coreengine_ffi`, platform stubs, and viewer targets.

---

## 1. Logical modules under `core/`

Registry: [modules/registry.json](modules/registry.json) (13 modules). Subfolders `Behaviors/`, `Conditions/`, and `MapComponents/` belong to their parent module.

| Module ID | Path | Role | OBJECT lib | Include enforced |
|-----------|------|------|------------|------------------|
| `core.math` | `core/math/` | Vectors (`Vector2`, `Vector3`), constants, deterministic RNG | yes | yes |
| `core.influence.types` | `core/influence_types/` | `InfluenceFieldItem` value types | yes | yes |
| `core.interfaces` | `core/interfaces/` | Cross-system contracts | *(headers only)* | yes |
| `core.logging` | `core/logging/` | `VenusWars::Logging::Logger`, levels, macros | yes | yes |
| `core.influence` | `core/influence/` | APF (`InfluenceField::CalculateTotalForce`), territory | yes | yes |
| `core.gameloop` | `core/gameloop/` | `GameLogicEngine`, deployment/think timers | yes | yes |
| `core.map` | `core/map/` | ECS map, placement validation | yes | yes |
| `core.ai` | `core/ai/` | FSM, `BehaviorFactory`, behaviors/conditions | yes | yes |
| `core.collision` | `core/collision/` | Spatial grid, detection, resolution | yes | yes |
| `core.combat` | `core/combat/` | Selection, commands, explosions; `CombatManager` stub | yes | yes |
| `core.serialization` | `core/serialization/` | JSON snapshot serializer | yes | yes |
| `core.gamestate` | `core/gamestate/` | `RTSGameState` hub, `RTSUnit` | yes | yes |
| `core.ai_advanced` | `core/ai_advanced/` | Genome / bot trainer (experimental) | *(in monolithic glob)* | **manifest only** |

### Related trees (outside `core/`)

| Area | Path | Relation to `core/` |
|------|------|---------------------|
| **Platform** | `platform/{null,sdl2,raylib,unity,unreal}/` | Header sketches; SDL2/Raylib **`Render()` unimplemented**, no `.cpp` in-repo. `BUILD_SDL2` / `BUILD_RAYLIB` **fail configure** with an explicit stub message (before `find_package`) until implementations exist — they do **not** ship working adapter libraries today. Runnable SDL2 window: `tools/viewer` + `COREENGINE_BUILD_SDL2_VIEWER`. No `#include` from `core/` into platform (verified). |
| **FFI** | `ffi/c_api.{h,cpp}` + `ffi/python`, `ffi/lua` | Shared lib `coreengine_ffi` when `BUILD_FFI=ON`; links **private** to monolithic `coreengine`. Public boundary is C types in `c_api.h`. |
| **Tests** | `tests/UnitTests/<Area>/`, `tests/IntegrationTests/` | Single executable `coreengine_tests` when `BUILD_TESTS=ON`; links `coreengine` + GTest. Mirrors core areas (Math, AI, Collision, …). |
| **Scripts** | `scripts/run-*-module-tests.sh` | Eleven standalone runners (no `gamestate` runner — use full `ctest`). |
| **Docs** | `docs/ARCHITECTURE.md`, guides | Authoritative subsystem descriptions; coding rules in `docs/CODING_GUIDELINES.md`. |

---

## 2. Build and test entry points

### CMake options (`CMakeLists.txt`)

| Option | Default | Output |
|--------|---------|--------|
| (none) | — | Static library `coreengine` (OBJECT libs per module) |
| `BUILD_FFI` | OFF | Shared `coreengine_ffi` |
| `BUILD_TESTS` | OFF | `coreengine_tests` + `ctest` discovery (299 tests) |
| `BUILD_SDL2` | OFF | Intended `coreengine_sdl2` static lib; **configure fails** with stub message while headers-only (no `.cpp`) |
| `BUILD_RAYLIB` | OFF | Intended `coreengine_raylib` static lib; **configure fails** with stub message while headers-only (no `.cpp`) |
| `COREENGINE_BUILD_SDL2_VIEWER` | OFF | `coreengine_sdl2_viewer` + optional viewer tests (needs SDL2; `libsdl2-dev`) |
| `COREENGINE_<MODULE>_STANDALONE` | OFF | Per-module test executable + narrowed link closure (see each `MODULE.md`) |

**GitLab CI `build-and-test` job** (not the full local gate):

```bash
./scripts/check-module-deps.sh
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --output-on-failure
```

**Recommended local merge gate** — [BUILD_INTEGRATION.md](BUILD_INTEGRATION.md) and [AGENTS.md](../../AGENTS.md#verification) (full `ctest`, all 11 standalone runners, guard scripts, doc links).

**Agentic dependency check** (manifests, committed `docs/deps/*`, `#include` boundaries, `core.interfaces` changelog heading, standalone link closure):

```bash
./scripts/check-module-deps.sh
python3 scripts/test_check_module_deps.py
```

### Standalone module runners (11)

| Script | Module |
|--------|--------|
| `./scripts/run-math-module-tests.sh` | `core.math` |
| `./scripts/run-influence-types-module-tests.sh` | `core.influence.types` |
| `./scripts/run-influence-module-tests.sh` | `core.influence` |
| `./scripts/run-logging-module-tests.sh` | `core.logging` |
| `./scripts/run-interfaces-module-tests.sh` | `core.interfaces` |
| `./scripts/run-gameloop-module-tests.sh` | `core.gameloop` |
| `./scripts/run-map-module-tests.sh` | `core.map` |
| `./scripts/run-ai-module-tests.sh` | `core.ai` |
| `./scripts/run-serialization-module-tests.sh` | `core.serialization` |
| `./scripts/run-collision-module-tests.sh` | `core.collision` |
| `./scripts/run-combat-module-tests.sh` | `core.combat` |

There is **no** `run-gamestate-module-tests.sh`; validate `core.gamestate` via full `ctest` and manifest include rules.

**FFI (local, not CI):** `./scripts/verify-ffi-build.sh`

### CI (`.gitlab-ci.yml`)

| Job | Purpose |
|-----|---------|
| `agentic-module-manifests` | Manifest + checker scripts (Python only image) |
| `math-module-pilot` | Math standalone runner |
| `influence-module-pilot` | Influence standalone runner |
| `logging-module-pilot` | Logging standalone runner |
| `build-and-test` | Full engine compile + `ctest` |

Other standalone runners and `verify-ffi-build.sh` are **local-only** today.

### Manifest checker limits

- Stores semver **ranges** in each module's `requires` map; does **not** evaluate whether a dependency's `version` satisfies the range.
- Patch / minor / major bump rules in [AGENTS.md](../../AGENTS.md) are policy for humans and agents, not automated classification.
- **`core.ai_advanced`:** listed in `registry.json` but **not** in `INCLUDE_ENFORCED_MODULES` (manifest validation only).

---

## 3. Coupling hotspots and policy gaps

### Intended layering (from `docs/CODING_GUIDELINES.md`)

- `core/interfaces/` is the **contract** layer; concrete systems should depend on interfaces, not peer concrete types.
- **No platform headers** in `core/` (holds today).

### Observed violations / tight coupling

| Issue | Files | Notes |
|-------|-------|-------|
| **Collision → `IUnitKinematics` gate** | `CollisionSystem.cpp`, `CollisionResolver.cpp`, … | No `#include` of `RTSUnit` in `core/collision` (R1.3). |
| **Combat → `IUnitAgent`** | `CommandSystem.cpp` | No `RTSUnit` in `core/combat` (R1.6). |
| **Serialization → hub** | `GameStateSerializer.cpp` | Depends on `RTSGameState` / snapshot interfaces. |
| **Hub module** | `RTSGameState.h` | Integration point — edit only in gamestate-scoped tasks. |
| **Interface completeness** | `IBehavior.h` | Fixed: full `InfluenceFieldItem` include ([BUILD_INTEGRATION.md](BUILD_INTEGRATION.md)). |

### Dependency sketch (manifest `requires`, target state)

See committed [docs/deps/graph.json](../deps/graph.json) and [SUMMARY.md](../deps/SUMMARY.md); layered policy in [DEPENDENCY_POLICY.md](DEPENDENCY_POLICY.md).

---

## 4. Test coverage by module

| Module | Unit tests (representative) | Integration / notes |
|--------|----------------------------|---------------------|
| `core.math` | `Vector2Test`, `Vector3Test`, `MathConstantsTest`, `DeterministicRNGTest` | `DeterminismTest` |
| `core.influence.types` | `InfluenceTypesModuleTest`, `InfluenceTypesCanonicalHeaderCompileTest`, legacy header compiles | — |
| `core.influence` | `InfluenceFieldTest`, legacy item/parameters header compiles | Standalone runner |
| `core.logging` | `LoggerTest`, `Logging*HeaderCompileTest` (4 files) | GitLab `logging-module-pilot` |
| `core.ai` | `StateMachineTest`, `BehaviorsTest`, `ConditionsTest`, `BehaviorFactoryTest`, `IBehaviorInterfaceTest`, `AIModuleHeaderCompileTest` | `BehaviorRuntimeSwapTest` |
| `core.collision` | `SpatialGridTest`, `ImmunityManagerTest`, `UnitKinematicsWriteTest`, `CollisionSystemKinematicsGateTest`, header compile | Standalone runner |
| `core.gameloop` | `GameLogicEngineTest`, `DeploymentTimerTest`, `ThinkTimerTest`, `GameloopInvariantsTest`, header compile | Standalone runner |
| `core.map` | `MapTest`, `MapModuleHeaderCompileTest` | Standalone runner |
| `core.combat` | `ExplosionTest`, `CommandSystemTest`, `CommandSystemAgentContractTest`, header compile | Standalone runner |
| `core.gamestate` | `GamestateModuleHeaderCompileTest`, `RTSGameStateNonRTSUnitGateTest`, `RTSGameStateCharacterizationTest` | `DeterminismTest`; **no** standalone runner |
| `core.serialization` | `SerializationModuleHeaderCompileTest`, `GameStateSerializerCharacterizationTest`, `GameStateSerializerSnapshotFakeTest` | Standalone runner |
| `core.interfaces` | `InterfacesModuleHeaderCompileTest`, `IGameLogicRendererDefaultSnapshotTest`, `GameStateInterfaceForwardCompileTest` | Standalone runner |
| `core.ai_advanced` | — | Manifest only |

---

## 5. Phase 2 pilot module recommendation

**Completed pilots:** `core.math`, `core.influence.types`, `core.influence`, `core.logging` (each has `MODULE.md`, checker opt-in, and a standalone runner except types/influence share CI partial coverage).

**Runner-up for future extraction work:** `core.gameloop` or `core.map` — clear boundaries, standalone runners already exist.

**`core.gamestate`:** include enforcement active (R2.1) — [core/gamestate/MODULE.md](../../core/gamestate/MODULE.md). Validate via full-engine tests, not a dedicated runner script.

---

## 6. Local build note (agent environment)

On Ubuntu 24.04 with GCC 13, configure with `CXX=g++` if the default `c++` symlink lacks `libstdc++`. Full `BUILD_TESTS=ON` build and `ctest` are the integration gate — see [BUILD_INTEGRATION.md](BUILD_INTEGRATION.md).
