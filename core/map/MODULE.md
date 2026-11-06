# Module: `core.map`

Manifest: [docs/agentic/modules/core.map.module.json](../../docs/agentic/modules/core.map.module.json)

## Purpose

Grid-free **placement and map entities**: dimensions, validity checks, entity containers, and attachable map components (resources, obstacles with stored `InfluenceFieldItem`). This module does not own rendering, pathfinding, collision resolution, or gamestate orchestration.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/map/Map.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `Map` | class | `Map.h` | Dimensions, shape, entity list — `.cpp` in this module |
| `MapEntity` | class | `MapEntity.h` | Id + position + components — `.cpp` in this module |
| `PlacementValidator` | class | `PlacementValidator.h` | Placement rules — `.cpp` in this module |
| `GameMode` | _(enum / header-only)_ | `GameMode.h` | Match mode tag used by higher layers |
| `MapObstacleComponent` | class | `MapComponents/MapObstacleComponent.h` | Obstacle radius + `InfluenceFieldItem` |
| `MapResourceComponent` | class | `MapComponents/MapResourceComponent.h` | Resource placement helper |

### Contract highlights

- **Canonical influence type include:** use `#include "core/influence_types/InfluenceFieldItem.h"` (not the legacy `core/influence/InfluenceFieldItem.h` wrapper) so the manifest `requires` matches actual `#include` boundaries.

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.math` | `^1.0.0` | `Vector2` and spatial queries |
| `core.interfaces` | `^1.0.0` | Reserved for future ECS-facing contracts; not required by current sources |
| `core.influence.types` | `^1.0.0` | `InfluenceFieldItem` on obstacles |

Allowed `#include` in `core/map/**`: standard library (`<…>`), headers under `core/map/`, or paths under `requires` modules (`core/math/…`, `core/interfaces/…`, `core/influence_types/…`). No other `core/*` siblings, `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/map/**` |
| Manifest | `docs/agentic/modules/core.map.module.json` |
| Unit tests | `tests/UnitTests/Map/**` |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

Do not read other `core/*` implementation trees for map-only tasks.

## Invariants

- Do not introduce `#include` of other CoreEngine modules without updating manifest `requires`, dependency policy, and regenerating `docs/deps/*`.
- Keep public includes stable at `core/map/…` (repo-root include path).

## Tests

| Suite | Path |
|-------|------|
| Map behavior | `tests/UnitTests/Map/MapTest.cpp` |
| Header compile smoke | `tests/UnitTests/Map/MapModuleHeaderCompileTest.cpp` |

### Map-only (OBJECT lib + math + influence types; no full `coreengine` static lib)

```bash
chmod +x scripts/run-map-module-tests.sh
./scripts/run-map-module-tests.sh
```

Equivalent manual steps:

```bash
cmake -S . -B build-map -G Ninja -DBUILD_TESTS=ON -DBUILD_FFI=OFF -DCOREENGINE_MAP_STANDALONE=ON
cmake --build build-map --target coreengine_map_tests
ctest --test-dir build-map --output-on-failure
```
