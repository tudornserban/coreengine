# Full-engine build integration gate

After the **IBehavior / InfluenceFieldItem compile repair**, a green full-engine build is the required gate before further module rollout (Phase 2+ extractions).

## Standard full suite (matches `.gitlab-ci.yml`)

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
```

## Scoped repair (2026-11-05)

| Item | Detail |
|------|--------|
| **Symptom** | GCC 13: `std::vector<InfluenceFieldItem>` in `IBehavior.h` with forward-declared `InfluenceFieldItem` — incomplete type in vector destructor |
| **Fix** | `#include "../influence_types/InfluenceFieldItem.h"` in `core/interfaces/IBehavior.h`; `core.interfaces` requires `core.influence.types` (not `core.influence`) |
| **Follow-on** | `ImmunityManager.cpp` missing `#include <vector>` (latent; exposed once AI compiled) |
| **Regression** | `tests/UnitTests/AI/IBehaviorInterfaceTest.cpp` — default `GenerateFieldItems()` on a minimal `IBehavior` impl |

Behavior and public API unchanged; only include completeness and documented dependency.

## Integration with `main`

Math pilot (PR #2) is merged on `main` (`fd718ef`). This repair lands on top via PR #3; after merge, the commands above validate the combined tree on `main`.

GitHub mirror PR merges do not sync to GitLab `gamedev4680743/coreengine`.

## Next module step (after gates)

With full CI green on `main`: **`core.logging`** is a possible future Phase 2 target (not started in this PR) — same MODULE.md + checker opt-in pattern as `core.math`.
