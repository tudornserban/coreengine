# Module: `core.combat`

Manifest: [docs/agentic/modules/core.combat.module.json](../../docs/agentic/modules/core.combat.module.json)

## Purpose

**Selection, player commands, explosion impulses, and combat manager stub** for CoreEngine. This module does not own game state orchestration or concrete unit types; it talks to units through `IGameUnit` and `IUnitAgent` only.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/combat/CommandSystem.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `CombatManager` | class | `CombatManager.h` | Stub — extend in game layer |
| `CommandSystem` | class | `CommandSystem.h` | `.cpp` dispatches via `IUnitAgent` where applicable |
| `SelectionSystem` | class | `SelectionSystem.h` | Box and click selection |
| `Explosion` | class | `Explosion.h` | Impulse math from position and radius |

### Contract highlights

- **Agent dispatch:** `CommandSystem` uses `dynamic_cast<IUnitAgent*>` for state and parameter commands; callers must not assume every `IGameUnit` implements `IUnitAgent`.
- **No `RTSUnit`:** do not include gamestate unit types from this module; use `IGameUnit` / `IUnitAgent` only.

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.math` | `^1.0.0` | `Vector2` |
| `core.interfaces` | `^1.2.0` | `IGameUnit`, `IUnitAgent` |

Allowed `#include` in `core/combat/**`: standard library (`<…>`), headers under `core/combat/`, or paths under `requires` modules (`core/math/…`, `core/interfaces/…`). No other `core/*` siblings, `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/combat/**` |
| Manifest | `docs/agentic/modules/core.combat.module.json` |
| Unit tests | `tests/UnitTests/Combat/**` (standalone + full-engine) |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

Do not read other `core/*` implementation trees for combat-only tasks.

## Invariants

- Do not introduce `#include` of other CoreEngine modules without updating manifest `requires`, dependency policy, and regenerating `docs/deps/*`.
- Keep public includes stable at `core/combat/…` (repo-root include path).

## Tests

| Suite | Path |
|-------|------|
| Explosion | `tests/UnitTests/Combat/ExplosionTest.cpp` |
| Command system (`IUnitAgent` contract) | `tests/UnitTests/Combat/CommandSystemAgentContractTest.cpp` |
| Command system (RTS delegation) | `tests/UnitTests/Combat/CommandSystemTest.cpp` (full-engine only) |
| Header compile smoke | `tests/UnitTests/Combat/CombatModuleHeaderCompileTest.cpp` |

### Combat-only (`COREENGINE_COMBAT_STANDALONE`)

```bash
./scripts/run-combat-module-tests.sh
```

Or manually:

```bash
cmake -S . -B build-combat -G Ninja -DBUILD_TESTS=ON -DBUILD_FFI=OFF -DCOREENGINE_COMBAT_STANDALONE=ON
cmake --build build-combat --target coreengine_combat_tests
ctest --test-dir build-combat --output-on-failure
```

Standalone links `coreengine_combat` and `coreengine_math` only (header-only `core.interfaces` via include paths).
