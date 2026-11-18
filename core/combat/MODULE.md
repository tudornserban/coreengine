# Module: `core.combat`

Manifest: [docs/agentic/modules/core.combat.module.json](../../docs/agentic/modules/core.combat.module.json)

## Purpose

**Selection, player commands, explosion impulses, and simultaneous combat resolution** for CoreEngine. This module does not own game state orchestration or concrete unit types; it talks to units through `IGameUnit`, `IUnitCombat`, `IInfluenceEmitter`, and `IUnitAgent` only.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/combat/CommandSystem.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `CombatManager` | class | `CombatManager.h` | Influence attrition, direct attack, and hybrid; `.cpp` in this module |
| `CommandSystem` | class | `CommandSystem.h` | `.cpp` dispatches via `IUnitAgent` where applicable |
| `SelectionSystem` | class | `SelectionSystem.h` | Box and click selection |
| `Explosion` | class | `Explosion.h` | Mass-scaled velocity change from position and radius |
| `ProjectileSimulation` | class | `ProjectileSimulation.h` | Swept projectiles and intercept. Games own firing policy |
| `ProjectileTargetMotion` | struct | `ProjectileSimulation.h` | Explicit start/end target motion for swept advances |
| `ProjectileImpactRecord` | struct | `ProjectileSimulation.h` | Attributed impact metadata from `LastImpacts()` |
| `InteractionMatrix` | class | `InteractionMatrix.h` | Move dominance and archetype damage multipliers |
| `TurnClashResolver` | class | `TurnClashResolver.h` | Simultaneous turn clash resolution |

### Contract highlights

- **Agent dispatch:** `CommandSystem` uses `dynamic_cast<IUnitAgent*>` for state and parameter commands; callers must not assume every `IGameUnit` implements `IUnitAgent`.
- **No `RTSUnit`:** do not include gamestate unit types from this module; use `IGameUnit` / `IUnitAgent` only.

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.math` | `^1.0.0` | `Vector2` |
| `core.interfaces` | `^1.7.0` | `IGameUnit` entity id, `IUnitAgent`, `IUnitCombat`, `IInfluenceEmitter` |

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

## Projectile interval and events (1.3.0)

`Advance(dt, units)` treats positions as interval starts and predicts constant-velocity
ends. `AdvanceSwept(dt, motions)` accepts explicit start/end positions for that same
interval. Target paths are linear chords, not reconstructed contact trajectories.
Dead units (HP <= 0) and units with collision participation disabled cannot consume
shots. Eligibility is checked again when resolving time/id-ordered impacts: a target
killed by an earlier shot no longer blocks a later shot.

`LastImpacts()` exposes `ProjectileImpactRecord` values with projectile, owner, team
and target IDs, contact time relative to the interval start, projectile-center contact
point, requested damage and actual nonnegative HP removed (armor and overkill excluded).
Damage is applied once through `IUnitCombat::TakeDamage`. Copy records for retained
history; the next advance or restore clears them. Existing `ProjectileImpact` raw
pointers returned by the direct API remain call-scoped.
