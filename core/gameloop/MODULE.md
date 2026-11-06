# Module: `core.gameloop`

Manifest: [docs/agentic/modules/core.gameloop.module.json](../../docs/agentic/modules/core.gameloop.module.json)

## Purpose

Portable, platform-agnostic **timing and playback** helpers for the simulation loop: fixed-timestep logic stepping, think-time banks, deployment charge timers, and lightweight match metadata (`MatchState`). This module does not own rendering, AI, map, collision, combat, or gamestate integration.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/gameloop/GameLogicEngine.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `GameLogicEngine` | class | `GameLogicEngine.h` | Fixed timestep, pause, speed, fast-forward — `.cpp` in this module |
| `PlaybackController` | class | `PlaybackController.h` | Thin wrapper over `GameLogicEngine` |
| `ThinkTimer` | class | `ThinkTimer.h` | Real-time think bank; header-only API |
| `DeploymentTimer` | class | `DeploymentTimer.h` | Charge / spend — `.cpp` in this module |
| `MatchState` / `MatchResult` | struct / enum | `MatchState.h` | Match metadata; no engine deps |

### Contract highlights

- **No hidden globals:** timers and engines are value or instance state only.
- **Forward-only contracts:** `GameLogicEngine` forward-declares `IGameState` but does not include interface headers (callers link types at a higher layer).

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.math` | `^1.0.0` | Manifest edge reserved for future numeric helpers; sources today use only the standard library |

Allowed `#include` in `core/gameloop/**`: standard library (`<…>`), headers under `core/gameloop/`, or `core/math/…` paths covered by `requires`. No other `core/*` siblings, `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/gameloop/**` |
| Manifest | `docs/agentic/modules/core.gameloop.module.json` |
| Unit tests | `tests/UnitTests/Gameloop/**` |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

Do not read other `core/*` implementation trees for gameloop-only tasks.

## Invariants

- Do not introduce `#include` of other CoreEngine modules without updating manifest `requires`, dependency policy, and regenerating `docs/deps/*`.
- Keep public includes stable at `core/gameloop/…` (repo-root include path).

**Timer / deployment notes (for tests and callers):**

- `ThinkTimer::Update` only drains the bank while `IsActive()` is true; when inactive, `GetBankRemaining()` is unchanged by `Update`.
- `DeploymentTimer::GetChargeProgress` is in `[0, 1]` whether or not a charge is banked; after `SpendCharge()` with no charges left, progress is again the partial fill toward the next charge.

## Tests

| Suite | Path |
|-------|------|
| Header compile smoke | `tests/UnitTests/Gameloop/GameloopModuleHeaderCompileTest.cpp` |
| Invariants (timers) | `tests/UnitTests/Gameloop/GameloopInvariantsTest.cpp` |

### Gameloop-only (OBJECT lib + math; no full `coreengine` static lib)

```bash
chmod +x scripts/run-gameloop-module-tests.sh
./scripts/run-gameloop-module-tests.sh
```

Equivalent manual steps:

```bash
cmake -S . -B build-gameloop -G Ninja -DBUILD_TESTS=ON -DCOREENGINE_GAMELOOP_STANDALONE=ON
cmake --build build-gameloop --target coreengine_gameloop_tests
ctest --test-dir build-gameloop --output-on-failure
```
