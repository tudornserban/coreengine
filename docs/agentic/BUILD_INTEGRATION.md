# Full-engine build integration gate

After the **IBehavior / InfluenceFieldItem compile repair**, a green full-engine build is the required gate before further module rollout (Phase 2+ extractions).

## Local full gate (recommended before merge)

Run from the **repository root** (do not `cd build` before `./scripts/…`):

```bash
export CXX="${CXX:-g++}"   # Ubuntu cloud images: default c++ may lack libstdc++
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./scripts/check-module-deps.sh
python3 scripts/test_check_module_deps.py
./scripts/run-math-module-tests.sh
./scripts/run-influence-module-tests.sh
./scripts/run-influence-types-module-tests.sh
./scripts/run-logging-module-tests.sh
./scripts/run-interfaces-module-tests.sh
./scripts/run-gameloop-module-tests.sh
./scripts/run-map-module-tests.sh
./scripts/run-ai-module-tests.sh
./scripts/run-serialization-module-tests.sh
./scripts/run-collision-module-tests.sh
./scripts/run-combat-module-tests.sh
```

`./scripts/check-module-deps.sh` also runs `scripts/check_docs_links.py` and `scripts/test_check_docs_links.py`.

This list is **stricter than GitLab CI** (see below): CI does not run every standalone module runner, the doc-link checker's pytest module in isolation (it is invoked inside `check-module-deps.sh`), or FFI verification.

### Not in GitLab CI (local / release hygiene)

```bash
./scripts/verify-ffi-build.sh   # BUILD_FFI=ON shared library + Python ctypes smoke
```

There is **no** `./scripts/run-gamestate-module-tests.sh`; `core.gamestate` is covered by the monolithic `coreengine_tests` / `ctest` and include enforcement only.

## What `.gitlab-ci.yml` runs today

| Job | What it executes |
|-----|------------------|
| `agentic-module-manifests` | `check-module-deps.sh`, `test_check_module_deps.py` |
| `math-module-pilot` | `check-module-deps.sh`, `run-math-module-tests.sh` |
| `influence-module-pilot` | `check-module-deps.sh`, `run-influence-module-tests.sh` |
| `logging-module-pilot` | `check-module-deps.sh`, `run-logging-module-tests.sh` |
| `build-and-test` | `check-module-deps.sh`, full configure/build, `ctest` (299 tests) |

**Not in CI:** `run-influence-types-module-tests.sh`, `run-interfaces-module-tests.sh`, `run-gameloop-module-tests.sh`, `run-map-module-tests.sh`, `run-ai-module-tests.sh`, `run-serialization-module-tests.sh`, `run-collision-module-tests.sh`, `run-combat-module-tests.sh`, `verify-ffi-build.sh`, optional SDL2 viewer builds.

## FFI shared library (`BUILD_FFI=ON`)

Linking `coreengine_ffi` pulls in the monolithic static `coreengine` archive and OBJECT-library objects; on ELF toolchains those inputs must be built with position-independent code. When `BUILD_FFI=ON`, root `CMakeLists.txt` sets `CMAKE_POSITION_INDEPENDENT_CODE ON` before any targets are defined.

```bash
./scripts/verify-ffi-build.sh
```

Default `BUILD_FFI=OFF` builds are unchanged (non-PIC static objects).

## Scoped repair (2026-11-05)

| Item | Detail |
|------|--------|
| **Symptom** | GCC 13: `std::vector<InfluenceFieldItem>` in `IBehavior.h` with forward-declared `InfluenceFieldItem` — incomplete type in vector destructor |
| **Fix** | `#include "../influence_types/InfluenceFieldItem.h"` in `core/interfaces/IBehavior.h`; `core.interfaces` requires `core.influence.types` (not `core.influence`) |
| **Follow-on** | `ImmunityManager.cpp` missing `#include <vector>` (latent; exposed once AI compiled) |
| **Regression** | `tests/UnitTests/AI/IBehaviorInterfaceTest.cpp` — default `GenerateFieldItems()` on a minimal `IBehavior` impl |

Behavior and public API unchanged; only include completeness and documented dependency.

## Integration with `main`

Math, logging, influence, and influence-types pilots are merged on `main`; module manifests and `COREENGINE_*_STANDALONE` runners follow the pattern documented in each module's `MODULE.md`.

GitHub mirror PR merges do not sync to GitLab `gamedev4680743/coreengine`.

## Module status

**`core.logging`** — Phase 2 complete (`MODULE.md`, checker opt-in, `./scripts/run-logging-module-tests.sh`, GitLab `logging-module-pilot` job).

**`core.influence.types`** — split foundation module with `./scripts/run-influence-types-module-tests.sh` (local full gate only; not a separate CI job yet).

**`core.ai_advanced`** — manifest entry only; no include enforcement and no standalone runner.
