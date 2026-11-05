# Phase 2 — `core.influence` + `core.influence.types`

**Status:** Open PR #4 — rebased on `main` @ `fc5c7e2` (includes merged PR #5 FFI PIC repair).

## Architectural outcome (Tudor option 2)

- New leaf **`core.influence.types`** (`core/influence_types/`): `InfluenceFieldItem`, `InfluenceFieldParameters`, `InfluenceFieldItem.cpp`.
- **`IBehavior`** includes `core/influence_types/InfluenceFieldItem.h` directly; **`core.interfaces` no longer requires `core.influence`**.
- **`core.influence`** owns calculator + territory `.cpp`; requires `core.influence.types`, `core.interfaces`, `core.math`.
- Legacy `#include "core/influence/InfluenceFieldItem.h"` remains a downward wrapper.
- **No** cycle allowlist; checker rejects all `requires` cycles.

## Dependency graph (manifest `requires`)

```text
core.influence.types  requires  core.math
core.interfaces       requires  core.math, core.influence.types
core.influence        requires  core.math, core.influence.types, core.interfaces
```

Edges are one-way; no cycle allowlist.

## Verification

Same commands as [BUILD_INTEGRATION.md](BUILD_INTEGRATION.md) plus:

```bash
./scripts/run-influence-types-module-tests.sh
./scripts/run-influence-module-tests.sh
```
