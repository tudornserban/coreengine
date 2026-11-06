# Phase 3 — `core.collision` include enforcement (R1.3 / A3.6)

`core.collision` participates in the same `#include` boundary checks as other opted-in modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/gamestate/…` without a manifest edge) fail `./scripts/check-module-deps.sh`.

**CMake:** `COREENGINE_COLLISION_STANDALONE=ON` builds the `coreengine_collision` OBJECT library plus collision gtests under `tests/UnitTests/Collision/` (some suites construct `RTSUnit`, so this target also links `coreengine_gamestate` and the sibling modules those sources need — collision itself stays manifest-clean). Includes `CollisionModuleHeaderCompileTest` (canonical `#include "core/collision/…"` compile smoke). Full-engine builds link collision via `$<TARGET_OBJECTS:coreengine_collision>` instead of globbing `core/collision/*.cpp` into the monolith.

**Agent working notes:** allowed `#include` surfaces are in `core/collision/MODULE.md` (`core/collision/…`, `core/math/…`, `core/interfaces/…`, `core/logging/…`, standard library). Prefer `./scripts/run-collision-module-tests.sh` before a full `ctest`.
