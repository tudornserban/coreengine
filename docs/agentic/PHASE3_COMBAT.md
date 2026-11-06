# Phase 3 — `core.combat` include enforcement (R1.6)

`core.combat` participates in the same `#include` boundary checks as other opted-in modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/gamestate/…` or `core/ai_advanced/…` without a manifest edge) fail `./scripts/check-module-deps.sh`.

Public headers use repo-root paths such as `#include "core/combat/CommandSystem.h"`; implementation uses canonical `#include "core/interfaces/…"` and `#include "core/math/…"` for declared `requires`.

**Agent working notes:** allowed `#include` surfaces are in `core/combat/MODULE.md` (`core/combat/…`, `core/math/…`, `core/interfaces/…`, standard library). No standalone combat-only CMake preset is required for this rollout; validate with `./scripts/check-module-deps.sh` and the full-engine `ctest` gate.
