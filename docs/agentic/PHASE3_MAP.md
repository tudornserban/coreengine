# Phase 3 — `core.map` include enforcement (A3.3)

**Status:** Include-path enforcement opt-in for the map module (sources and headers under `core/map/`). Manifest `requires` updated to declare the existing **`core.influence.types`** dependency used by `MapObstacleComponent` (`InfluenceFieldItem`). Sources now include `core/influence_types/InfluenceFieldItem.h` (canonical) instead of the legacy `core/influence/InfluenceFieldItem.h` wrapper so the graph matches real includes. No public API or behavior change.

`core.map` participates in the same `#include` boundary checks as other opted-in modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/logging/…` without a manifest edge) fail `./scripts/check-module-deps.sh`.

**CMake:** `COREENGINE_MAP_STANDALONE=ON` builds the `coreengine_map` OBJECT library plus Map gtests: `MapTest`, `MapModuleHeaderCompileTest`. Full-engine builds link map via `$<TARGET_OBJECTS:coreengine_map>` instead of globbing `core/map/*.cpp` into the monolith.

**Agent working notes:** allowed `#include` surfaces are in `core/map/MODULE.md` (`core/map/…`, `core/math/…`, `core/interfaces/…`, `core/influence_types/…`, standard library). Prefer `./scripts/run-map-module-tests.sh` before a full `ctest`.

**SemVer:** Enforcement-only and manifest bookkeeping; `core.map` manifest version left at `1.0.0` (no changelog entry required).
