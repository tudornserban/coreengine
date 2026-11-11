# Phase 3 — `core.combat` include enforcement (R1.6)

`core.combat` participates in the same `#include` boundary checks as other opted-in modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/gamestate/…` or `core/ai_advanced/…` without a manifest edge) fail `./scripts/check-module-deps.sh`.

Public headers use repo-root paths such as `#include "core/combat/CommandSystem.h"`; implementation uses canonical `#include "core/interfaces/…"` and `#include "core/math/…"` for declared `requires`.

**CMake:** `COREENGINE_COMBAT_STANDALONE=ON` builds the `coreengine_combat` OBJECT library plus combat gtests under `tests/UnitTests/Combat/`, linking only `coreengine_combat`, `coreengine_math` (plus GTest). `CommandSystemAgentContractTest` locks `IUnitAgent` forwarding without `RTSUnit` or AI headers; RTS delegation remains in `CommandSystemTest`. Includes `CombatModuleHeaderCompileTest` (canonical `#include "core/combat/…"` compile smoke). Full-engine builds link combat via `$<TARGET_OBJECTS:coreengine_combat>` instead of globbing `core/combat/*.cpp` into the monolith.

**Agent working notes:** allowed `#include` surfaces are in `core/combat/MODULE.md` (`core/combat/…`, `core/math/…`, `core/interfaces/…`, standard library). Combat-only verification:

```bash
./scripts/run-combat-module-tests.sh
```

Also run `./scripts/check-module-deps.sh` and the full-engine `ctest` gate when touching manifests or cross-module edges.
