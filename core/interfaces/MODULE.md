# Module: `core.interfaces`

Manifest: [docs/agentic/modules/core.interfaces.module.json](../../docs/agentic/modules/core.interfaces.module.json)

## Purpose

Published **contract headers** (pure virtual interfaces) for units, behaviors, input, serialization, map components, and high-level game flow. This package is **header-only**: no translation units under `core/interfaces/`. It does **not** own implementations, platform adapters, influence-field calculation, or game-state concrete types.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/interfaces/IGameUnit.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `IBehavior` | class | `IBehavior.h` | Influence items + optional direct acceleration; includes value types from `core.influence.types` and `Vector2` |
| `IGameUnit` | class | `IGameUnit.h` | Unit contract; uses `Vector2` |
| `IUnitKinematics` | class | `IUnitKinematics.h` | Optional direct position/velocity writes (`SetPosition`, `SetVelocity`); implemented by `RTSUnit` for collision writers |
| `IUnitAgent` | class | `IUnitAgent.h` | Optional FSM / behavior-parameter commands (`SetAgentState`, `SetAgentParameter`); implemented by `RTSUnit` for `CommandSystem` |
| `IInputReader` | class | `IInputReader.h` | Input abstraction; uses `Vector2` |
| `ITransitionCondition` | class | `ITransitionCondition.h` | Condition evaluation; `BehaviorContext` forward-declared |
| `IGameState` | class | `IGameState.h` | Single active state contract |
| `IGameLogicRenderer` | class | `IGameLogicRenderer.h` | Read-only renderer; `GetCurrentGameState` (`const RTSGameState*`); optional `GetSimulationSnapshot()` (default `nullptr`) |
| `ISerializer` | class | `ISerializer.h` | Byte/string serialization contract |
| `IMapComponent` | class | `IMapComponent.h` | Map ECS component contract |
| `ISimulationSnapshot` | class | `ISimulationSnapshot.h` | Read-only map size + unit list for snapshot serializers |
| `IUnitBehaviorHost` | class | `IUnitBehaviorHost.h` | Active behavior accessor for hub AI tick; implemented by `RTSUnit` |

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.math` | `^1.0.0` | `Vector2` in `IGameUnit`, `IBehavior`, `IInputReader` |
| `core.influence.types` | `^1.0.0` | `InfluenceFieldItem` in `IBehavior` |

Allowed `#include` in `core/interfaces/**`: standard library, headers under `core/interfaces/`, and (via manifest `requires`) headers under `core/math/` and `core/influence_types/`. Relative includes must resolve inside those trees or this module. No other `core/*` siblings, `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Contract headers | `core/interfaces/**` |
| Manifest | `docs/agentic/modules/core.interfaces.module.json` |
| Changelog | `core/interfaces/CHANGELOG.md` (when manifest version changes) |
| Compile smoke tests | `tests/UnitTests/Interfaces/**` |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

## Invariants

- Do not add concrete game types or pull in systems modules (`core.influence`, `core.gamestate`, etc.) without a dedicated extraction and manifest update.
- Preserve stable include paths and pure-virtual surface unless intentionally breaking semver major.

## Tests

| Suite | Path |
|-------|------|
| Header compile (canonical includes) | `tests/UnitTests/Interfaces/InterfacesModuleHeaderCompileTest.cpp` |

Full build:

```bash
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure -R InterfacesModuleHeaderCompileTest
```

## Change checklist

- [ ] Update `docs/agentic/modules/core.interfaces.module.json` version if exports or observable contract changes
- [ ] Append `core/interfaces/CHANGELOG.md` under `## [same version as manifest]`
- [ ] Run `./scripts/check-module-deps.sh`