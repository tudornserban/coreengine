# Phase 3 — `core.gameloop` include enforcement (A3.2)

**Status:** Include-path enforcement opt-in for the gameloop module (sources and headers under `core/gameloop/`). Manifest `requires` unchanged in this rollout: `core.math` only. No public API or behavior change.

`core.gameloop` participates in the same `#include` boundary checks as other opted-in modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/logging/…` without a manifest edge) fail `./scripts/check-module-deps.sh`.

**CMake:** `COREENGINE_GAMELOOP_STANDALONE=ON` builds the `coreengine_gameloop` OBJECT library plus the Gameloop gtest target: `GameloopModuleHeaderCompileTest` (canonical `#include "core/gameloop/…"` compile smoke) and `GameloopInvariantsTest` (small behavioral locks on `ThinkTimer` / `DeploymentTimer`). Full-engine builds link gameloop via `$<TARGET_OBJECTS:coreengine_gameloop>` instead of globbing `core/gameloop/*.cpp` into the monolith.

**Agent working notes:** allowed `#include` surfaces are in `core/gameloop/MODULE.md` (`core/gameloop/…`, `core/math/…`, standard library). Public headers: `GameLogicEngine.h`, `PlaybackController.h`, `ThinkTimer.h`, `DeploymentTimer.h`, `MatchState.h` (see manifest `exports`). Prefer `./scripts/run-gameloop-module-tests.sh` before a full `ctest`.

**SemVer:** Enforcement-only; manifest version left at `1.0.0` (no changelog entry required).
