# Phase 3 — `core.gamestate` include enforcement (R2.1)

`core.gamestate` participates in the same `#include` boundary checks as other opted-in modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/serialization/…` or `core/ai_advanced/…` without a manifest edge) fail `./scripts/check-module-deps.sh`.

The production tree at the R2.1 baseline already satisfied the declared `requires` graph; this rollout adds enforcement, docs, checker regressions, and header compile smoke — **no** production include rewrites.

**Agent working notes:** allowed `#include` surfaces are in `core/gamestate/MODULE.md` (`core/gamestate/…`, sibling paths for the eight `requires` modules, standard library). Validate with `./scripts/check-module-deps.sh` and the full-engine `ctest` gate.
