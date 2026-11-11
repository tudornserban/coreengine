# Phase 3 — `core.ai` include enforcement (A3.4)

`core.ai` participates in the same `#include` boundary checks as other opted-in modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/gamestate/…` without a manifest edge) fail `./scripts/check-module-deps.sh`.

**CMake:** `COREENGINE_AI_STANDALONE=ON` builds the `coreengine_ai` OBJECT library plus AI gtests: existing suites under `tests/UnitTests/AI/` plus `AIModuleHeaderCompileTest` (canonical `#include "core/ai/…"` compile smoke). Full-engine builds link AI via `$<TARGET_OBJECTS:coreengine_ai>` instead of globbing `core/ai/**/*.cpp` into the monolith.

**Agent working notes:** allowed `#include` surfaces are in `core/ai/MODULE.md` (`core/ai/…`, `core/math/…`, `core/interfaces/…`, `core/logging/…`, `core/influence_types/…`, standard library). Prefer `./scripts/run-ai-module-tests.sh` before a full `ctest`.
