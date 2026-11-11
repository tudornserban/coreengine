# Phase 3 — `core.serialization` include enforcement (A3.5)

`core.serialization` participates in the same `#include` boundary checks as other opted-in modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/logging/…` without a manifest edge) fail `./scripts/check-module-deps.sh`.

**CMake:** `COREENGINE_SERIALIZATION_STANDALONE=ON` builds the `coreengine_serialization` OBJECT library plus `SerializationModuleHeaderCompileTest` (canonical `#include "core/serialization/…"` compile smoke). Full-engine builds link serialization via `$<TARGET_OBJECTS:coreengine_serialization>` instead of globbing `core/serialization/*.cpp` into the monolith.

**Agent working notes:** allowed `#include` surfaces are in `core/serialization/MODULE.md` (`core/serialization/…`, `core/gamestate/…`, `core/interfaces/…`, standard library). Prefer `./scripts/run-serialization-module-tests.sh` before a full `ctest`.
