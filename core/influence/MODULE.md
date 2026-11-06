# Module: `core.influence`

Manifest: [docs/agentic/modules/core.influence.module.json](../../docs/agentic/modules/core.influence.module.json)

Value types live in **[core.influence.types](../influence_types/MODULE.md)** (`core/influence_types/`).

## Purpose

APF **force calculation** and **territory** sampling. Item/value definitions are not owned here (see types leaf).

## Public surface (`exports`)

| Symbol | Header | Notes |
|--------|--------|-------|
| `InfluenceField` | `InfluenceFieldCalculator.h` | Force aggregation |
| `TerritoryCalculator` | `TerritoryCalculator.h` | Voronoi-style territory |

Compatibility wrappers (re-export types only): `core/influence/InfluenceFieldItem.h`, `InfluenceFieldParameters.h`.

## Dependencies (`requires`)

| Module | Why |
|--------|-----|
| `core.math` | `Vector2` |
| `core.influence.types` | Item types used by calculator |
| `core.interfaces` | `IGameUnit` in `TerritoryCalculator.cpp` |

Acyclic graph: `core.interfaces` → `core.influence.types` (via `IBehavior`); `core.influence` → `core.interfaces` (one-way).

## Tests

```bash
./scripts/run-influence-module-tests.sh
```

Standalone links `coreengine_influence_types` + `coreengine_influence` + `coreengine_math` object libraries (2 calculator/territory `.cpp` + 1 types `.cpp`).

See [BUILD_INTEGRATION.md](../../docs/agentic/BUILD_INTEGRATION.md).