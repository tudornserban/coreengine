# Module: `core.ai`

Manifest: [docs/agentic/modules/core.ai.module.json](../../docs/agentic/modules/core.ai.module.json)

## Purpose

Finite-state **AI behaviors and transition conditions** for units: `StateMachine`, concrete behaviors (seek, flee, wander, cohesion, separation), and `ITransitionCondition` implementations. This module does not own gamestate orchestration, collision, combat resolution, or map placement.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/ai/StateMachine.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `StateMachine` | class | `StateMachine.h` | FSM — `.cpp` in this module |
| `BehaviorContext` | struct | `BehaviorContext.h` | Per-tick context (nearby units, map vectors, static `InfluenceFieldItem` list, RNG pointer) |
| `BehaviorFactory` | class | `BehaviorFactory.h` | Registers behavior factories — `.cpp` in this module |
| `SeekBehavior`, `FleeBehavior`, `WanderBehavior`, `CohesionBehavior`, `SeparationBehavior` | class | `Behaviors/*.h` | Unit steering helpers |
| `EnemyInRangeCondition`, `AlliesNearbyCondition`, `TimeElapsedCondition` | class | `Conditions/*.h` | Transition predicates |

### Contract highlights

- **Canonical influence type include:** use `#include "core/influence_types/InfluenceFieldItem.h"` (not `core/influence/InfluenceFieldItem.h`) so manifest `requires` matches actual `#include` boundaries.
- **Interfaces:** depend on `IGameUnit`, `IBehavior`, `ITransitionCondition` from `core/interfaces/`.

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.math` | `^1.0.0` | `Vector2`, `DeterministicRNG`, constants |
| `core.interfaces` | `^1.0.0` | Behavior and unit contracts |
| `core.logging` | `^1.0.0` | `Logger` in `StateMachine` |
| `core.influence.types` | `^1.0.0` | `InfluenceFieldItem` in `BehaviorContext` |

Allowed `#include` in `core/ai/**`: standard library (`<…>`), headers under `core/ai/`, or paths under `requires` modules. No other `core/*` siblings, `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/ai/**` |
| Manifest | `docs/agentic/modules/core.ai.module.json` |
| Unit tests | `tests/UnitTests/AI/**` |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

Do not read other `core/*` implementation trees for AI-only tasks.

## Invariants

- Do not introduce `#include` of other CoreEngine modules without updating manifest `requires`, dependency policy, and regenerating `docs/deps/*`.
- Keep public includes stable at `core/ai/…` (repo-root include path).

## Tests

| Suite | Path |
|-------|------|
| AI behavior / FSM | `tests/UnitTests/AI/*Test.cpp` |
| Header compile smoke | `tests/UnitTests/AI/AIModuleHeaderCompileTest.cpp` |

### AI-only (OBJECT lib + math + influence types + logging; no full `coreengine` static lib)

```bash
chmod +x scripts/run-ai-module-tests.sh
./scripts/run-ai-module-tests.sh
```

Equivalent manual steps:

```bash
cmake -S . -B build-ai -G Ninja -DBUILD_TESTS=ON -DBUILD_FFI=OFF -DCOREENGINE_AI_STANDALONE=ON
cmake --build build-ai --target coreengine_ai_tests
ctest --test-dir build-ai --output-on-failure
```
