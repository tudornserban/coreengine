# Module: `core.serialization`

Manifest: [docs/agentic/modules/core.serialization.module.json](../../docs/agentic/modules/core.serialization.module.json)

## Purpose

**JSON-oriented game state snapshots** for `RTSGameState`: implements `ISerializer` so callers can exchange portable state strings or byte buffers (server validation, replay scaffolding, FFI helpers). This module does not own simulation, map layout, combat, or AI behavior.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/serialization/GameStateSerializer.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `GameStateSerializer` | class | `GameStateSerializer.h` | Implements `ISerializer` — `.cpp` in this module |

### Contract highlights

- **Interfaces:** depend on `ISerializer` from `core/interfaces/`.
- **Game state:** reads map size and units through `ISimulationSnapshot`; retains `GameStateSerializer(const RTSGameState*)` / `SetState` and the `core.gamestate` manifest edge (`.cpp` still includes `RTSGameState.h`).
- **Overload:** `GameStateSerializer(const ISimulationSnapshot*)` / `SetSnapshot` for non-hub snapshot sources.

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.gamestate` | `^1.0.0` | Serialize/deserialize against `RTSGameState` |
| `core.interfaces` | `^1.0.0` | `ISerializer` base contract |

Allowed `#include` in `core/serialization/**`: standard library (`<…>`), headers under `core/serialization/`, or paths under `requires` modules. No other `core/*` siblings, `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/serialization/**` |
| Manifest | `docs/agentic/modules/core.serialization.module.json` |
| Unit tests | `tests/UnitTests/Serialization/**` |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

## Invariants

- Do not introduce `#include` of other CoreEngine modules without updating manifest `requires`, dependency policy, and regenerating `docs/deps/*`.
- Keep public includes stable at `core/serialization/…` (repo-root include path).

## Tests

| Suite | Path |
|-------|------|
| Header compile smoke | `tests/UnitTests/Serialization/SerializationModuleHeaderCompileTest.cpp` |

### Serialization-only (OBJECT lib + `RTSGameState` hub closure; no full `coreengine` static lib)

```bash
chmod +x scripts/run-serialization-module-tests.sh
./scripts/run-serialization-module-tests.sh
```

Equivalent manual steps:

```bash
cmake -S . -B build-serialization -G Ninja -DBUILD_TESTS=ON -DBUILD_FFI=OFF -DCOREENGINE_SERIALIZATION_STANDALONE=ON
cmake --build build-serialization --target coreengine_serialization_tests
ctest --test-dir build-serialization --output-on-failure
```
