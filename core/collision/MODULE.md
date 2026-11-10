# Module: `core.collision`

Manifest: [docs/agentic/modules/core.collision.module.json](../../docs/agentic/modules/core.collision.module.json)

## Purpose

**Spatial queries, contact detection, impulse resolution, events, and immunity** for units. Writers resolve kinematics through `IUnitKinematics` when present; agent–agent and map-edge paths use `IGameUnit` for read-only queries. This module does not own gamestate orchestration, combat rules, or map placement.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/collision/CollisionSystem.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `CollisionSystem` | class | `CollisionSystem.h` | Orchestrates detection and resolution — `.cpp` in this module |
| `CollisionDetector` | class | `CollisionDetector.h` | Broad + narrow phase — `.cpp` in this module |
| `SpatialGrid` | class | `SpatialGrid.h` | Spatial hash — `.cpp` in this module |
| `CollisionResolver` | class | `CollisionResolver.h` | Impulse resolution — `.cpp` in this module |
| `PositionCorrector` | class | `PositionCorrector.h` | Positional correction — `.cpp` in this module |
| `ImmunityManager` | class | `ImmunityManager.h` | Pair immunity — `.cpp` in this module |
| `ICollisionDetector` | interface | `ICollisionDetector.h` | Detector contract |
| `ICollidable` | interface | `ICollidable.h` | Collidable contract |
| `ICollisionObserver` | interface | `ICollisionObserver.h` | Event observer |

### Contract highlights

- **Kinematics gate:** resolution paths that apply impulses or correct positions require `IUnitKinematics` (optional interface); callers must not assume every `IGameUnit` implements it.
- **No `RTSUnit`:** do not include gamestate unit types from this module; use `IGameUnit` / `IUnitKinematics` only.

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.math` | `^1.0.0` | `Vector2`, geometry |
| `core.interfaces` | `^1.0.0` | `IGameUnit`, `IUnitKinematics` |
| `core.influence.types` | `^1.0.0` | Transitive completeness when parsing interface headers (matches `core.interfaces` contract) |
| `core.logging` | `^1.0.0` | `Logger` in `CollisionSystem` |

Allowed `#include` in `core/collision/**`: standard library (`<…>`), headers under `core/collision/`, or paths under `requires` modules (`core/math/…`, `core/interfaces/…`, `core/influence_types/…`, `core/logging/…`). No other `core/*` siblings, `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/collision/**` |
| Manifest | `docs/agentic/modules/core.collision.module.json` |
| Unit tests | `tests/UnitTests/Collision/**` |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

Do not read other `core/*` implementation trees for collision-only tasks.

## Invariants

- Do not introduce `#include` of other CoreEngine modules without updating manifest `requires`, dependency policy, and regenerating `docs/deps/*`.
- Keep public includes stable at `core/collision/…` (repo-root include path).

## Tests

| Suite | Path |
|-------|------|
| Spatial grid | `tests/UnitTests/Collision/SpatialGridTest.cpp` |
| Immunity | `tests/UnitTests/Collision/ImmunityManagerTest.cpp` |
| Kinematics writes | `tests/UnitTests/Collision/UnitKinematicsWriteTest.cpp` |
| System gate | `tests/UnitTests/Collision/CollisionSystemKinematicsGateTest.cpp` |
| Header compile smoke | `tests/UnitTests/Collision/CollisionModuleHeaderCompileTest.cpp` |

### Collision-only (`COREENGINE_COLLISION_STANDALONE`)

Standalone target `coreengine_collision_tests` links `coreengine_collision`, `coreengine_logging`, `coreengine_influence_types`, and `coreengine_math` (plus GTest). Kinematics coverage uses `tests/TestHelpers/KinematicMockGameUnit.h` (`IUnitKinematics` test double) instead of `RTSUnit`.

```bash
chmod +x scripts/run-collision-module-tests.sh
./scripts/run-collision-module-tests.sh
```

Equivalent manual steps:

```bash
cmake -S . -B build-collision -G Ninja -DBUILD_TESTS=ON -DBUILD_FFI=OFF -DCOREENGINE_COLLISION_STANDALONE=ON
cmake --build build-collision --target coreengine_collision_tests
ctest --test-dir build-collision --output-on-failure
```
