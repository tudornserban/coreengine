# Module: `core.influence`

Manifest: [docs/agentic/modules/core.influence.module.json](../../docs/agentic/modules/core.influence.module.json)

Value types live in **[core.influence.types](../influence_types/MODULE.md)** (`core/influence_types/`).

## Purpose

APF **force calculation** and **territory** sampling. Item/value definitions are not owned here (see types leaf).

## Public surface (`exports`)

| Symbol | Header | Notes |
|--------|--------|-------|
| `InfluenceField` | `InfluenceFieldCalculator.h` | Force aggregation |
| `TerritoryCalculator` | `TerritoryCalculator.h` | 2-team `Calculate` (0–100) and N-team `UpdateGrid` |
| `ClusterDetector` | `ClusterDetector.h` | Same-team proximity components, wrap-aware |
| `EncirclementChecker` | `EncirclementChecker.h` | Border liberty flood-fill |
| `SurroundChecker` | `SurroundChecker.h` | Circumference surround samples |

Compatibility wrappers (re-export types only): `core/influence/InfluenceFieldItem.h`, `InfluenceFieldParameters.h`.

## Dependencies (`requires`)

| Module | Why |
|--------|-----|
| `core.math` | `Vector2` |
| `core.influence.types` | Item types used by calculator |
| `core.interfaces` | `IGameUnit`, `IInfluenceEmitter`, `IUnitCombat` |
| `core.map` | Shared `Obstacle` disc for territory and encirclement |

Acyclic graph: `core.influence` depends on `core.interfaces` and `core.map`; neither of those depends on `core.influence`.

## Tests

```bash
./scripts/run-influence-module-tests.sh
```

Standalone links `coreengine_influence`, `coreengine_influence_types`, and `coreengine_math`. `Obstacle` is header-only in `core.map`.

See [BUILD_INTEGRATION.md](../../docs/agentic/BUILD_INTEGRATION.md).