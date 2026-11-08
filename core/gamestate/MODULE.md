# Module: `core.gamestate`

Manifest: [docs/agentic/modules/core.gamestate.module.json](../../docs/agentic/modules/core.gamestate.module.json)

## Purpose

**Simulation hub** for RTS-style matches: unit list, map integration, selection and commands, collision and explosion handling, deterministic RNG, static influence fields, and `RTSUnit` as the concrete `IGameUnit`. **`GameStateManager`** owns menu/mission state stack and transitions between high-level game modes.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/gamestate/RTSGameState.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `RTSGameState` | class | `RTSGameState.h` | Main simulation step (`Update`, units, map, combat systems) |
| `RTSUnit` | class | `RTSUnit.h` | Concrete unit; AI FSM + kinematics/agent interfaces |
| `GameStateManager` | class | `GameStateManager.h` | Menu / mission stack |
| `IGameState` | header | `IGameState.h` | Legacy forwarder → `core/interfaces/IGameState.h` |
| `IGameLogicRenderer` | header | `IGameLogicRenderer.h` | Legacy forwarder → `core/interfaces/IGameLogicRenderer.h` |

### Contract highlights

- **`GetCurrentGameState`** returns `const RTSGameState*` (do not widen to non-const without an explicit API change).
- **`BehaviorContext::self`** remains `IGameUnit*`; gamestate wires concrete units without changing the AI contract.
- **`RTSGameState.cpp`** may include sibling `RTSUnit.h` (same module).

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.ai` | `^1.0.0` | `BehaviorContext`, unit AI tick |
| `core.combat` | `^1.0.0` | Selection, commands, explosions |
| `core.map` | `^1.0.0` | `Map`, placement/obstacle components |
| `core.influence` | `^1.0.0` | Influence items and calculator |
| `core.collision` | `^1.0.0` | Collision system / detector hooks |
| `core.math` | `^1.0.0` | `Vector2`, deterministic RNG |
| `core.logging` | `^1.0.0` | `Logger` |
| `core.interfaces` | `^1.0.0` | `IGameUnit`, kinematics, game-state contracts |

Allowed `#include` in `core/gamestate/**`: standard library (`<…>`), headers under `core/gamestate/`, repo-root paths under declared `requires` (e.g. `core/interfaces/…`), or **relative** includes that resolve into a declared requirement (e.g. `../logging/Logger.h`, `../combat/CommandSystem.h`). No other `core/*` siblings (`core/serialization/`, `core/ai_advanced/`, …), `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/gamestate/**` |
| Manifest | `docs/agentic/modules/core.gamestate.module.json` |
| Unit tests | `tests/UnitTests/Gamestate/**` |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

## Invariants

- **Legacy forwarders** in `IGameState.h` / `IGameLogicRenderer.h` remain; canonical contracts live under `core/interfaces/`.
- **`SetAgentState` / `SetAgentParameter`** forwarder bodies stay verbatim on `RTSUnit` / command paths.
- **`RTSGameState::Update`:** all three `dynamic_cast<RTSUnit*>` gates remain (AI behavior tick, explosion impulse application, bounds fallback when not handled by radius-aware collision push).
- Do not broaden non-`RTSUnit` behavior in those gates; bounds fallback is separate from collision radius-aware push.
- **Map-edge collision events** always emit, including for units that do not implement `IUnitKinematics`.
- Do not introduce `#include` of other CoreEngine modules without updating manifest `requires`, dependency policy, and regenerating `docs/deps/*`.

## Tests

| Suite | Path |
|-------|------|
| Header compile smoke | `tests/UnitTests/Gamestate/GamestateModuleHeaderCompileTest.cpp` |
| Determinism (integration) | `tests/IntegrationTests/DeterminismTest.cpp` (when present in full gate) |

Run with the full-engine test target (`cmake` + `ctest`).
