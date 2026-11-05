# Phase 2 — `core.influence` + `core.influence.types`

**Status:** Implemented on PR #4 (types leaf breaks interfaces↔influence cycle).  
**Base:** `main` @ `628ce572`.

## Architectural outcome (Tudor option 2)

- New leaf **`core.influence.types`** (`core/influence_types/`): `InfluenceFieldItem`, `InfluenceFieldParameters`, `InfluenceFieldItem.cpp`.
- **`IBehavior`** includes `core/influence_types/InfluenceFieldItem.h` directly; **`core.interfaces` no longer requires `core.influence`**.
- **`core.influence`** owns calculator + territory `.cpp`; requires `core.influence.types`, `core.interfaces`, `core.math`.
- Legacy `#include "core/influence/InfluenceFieldItem.h"` remains a downward wrapper.
- **No** cycle allowlist; checker rejects all `requires` cycles.

## Dependency graph (manifest)

```text
core.math
  ↑ core.influence.types
  ↑     ↑
  |     └── core.influence  → core.interfaces (one-way)
  └──────── core.interfaces
```

## Verification

Same commands as [BUILD_INTEGRATION.md](BUILD_INTEGRATION.md) + `./scripts/run-influence-module-tests.sh`.
