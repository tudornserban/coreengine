# Module: `core.influence`

Manifest: [docs/agentic/modules/core.influence.module.json](../../docs/agentic/modules/core.influence.module.json)

## Purpose

Artificial potential field (APF) primitives: influence items, force calculation, and Voronoi-style territory sampling. Owns movement **field** math, not unit AI state machines, collision, or game rules.

## Public surface (`exports`)

Include from repo root, e.g. `#include "core/influence/InfluenceFieldItem.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `InfluenceFieldItem` | class | `InfluenceFieldItem.h` | Attractors/repulsors/edges |
| `InfluenceFieldParameters` | structs | `InfluenceFieldParameters.h` | Default APF parameters |
| `InfluenceField` | namespace | `InfluenceFieldCalculator.h` | Force aggregation helpers |
| `TerritoryCalculator` | class | `TerritoryCalculator.h` | Grid territory percentages |

### Contract highlights

- **Determinism:** Pure functions of inputs (positions, items, parameters); no hidden RNG in this module.
- **Territory:** `TerritoryCalculator::Calculate` ignores units with team IDs other than 1 and 2; empty unit list → `{0, 0}` percentages.
- **Forces:** `CalculateTotalForce` clamps resultant acceleration magnitude to the supplied `maxAccel`.

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| `core.math` | `^1.0.0` | `Vector2` in items and force math |
| `core.interfaces` | `^1.0.0` | `IGameUnit` in `TerritoryCalculator.cpp` (header uses forward declare only) |

**Mutual edge with `core.interfaces`:** `IBehavior::GenerateFieldItems()` returns `std::vector<InfluenceFieldItem>` (see build repair on `main`). Manifests document both directions; dependency checker allows this pair only.

Allowed `#include` under `core/influence/**`: stdlib, `core/math/…`, `core/interfaces/…` (as declared), own headers, or quoted paths resolving inside `core/influence/`. No other `core/*` siblings, `platform/`, or `ffi/` (enforced when opted in).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/influence/**` |
| Manifest | `docs/agentic/modules/core.influence.module.json` |
| Unit tests | `tests/UnitTests/Influence/**` |
| Shared agentic docs | `docs/agentic/*` when required |

## Tests

| Suite | Path |
|-------|------|
| APF + territory | `tests/UnitTests/Influence/InfluenceFieldTest.cpp` |

### Influence-only (no sibling `core/` implementation `.cpp`)

```bash
chmod +x scripts/run-influence-module-tests.sh
./scripts/run-influence-module-tests.sh
```

Builds only `coreengine_math` + `coreengine_influence` object libraries plus the influence test executable (test helper headers only; no `coreengine` static lib).

Manual:

```bash
cmake -S . -B build-influence -G Ninja -DBUILD_TESTS=ON -DCOREENGINE_INFLUENCE_STANDALONE=ON
cmake --build build-influence --target coreengine_influence_tests
ctest --test-dir build-influence --output-on-failure
```

## Change checklist

- [ ] Update manifest version if exports or observable behavior changes
- [ ] Update this file if boundaries or commands change
- [ ] Run `./scripts/check-module-deps.sh` and `python3 scripts/test_check_module_deps.py`
- [ ] Run `./scripts/run-influence-module-tests.sh` and full engine gate per [BUILD_INTEGRATION.md](../../docs/agentic/BUILD_INTEGRATION.md)
