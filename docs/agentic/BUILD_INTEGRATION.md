# Full-engine build integration gate

After the **IBehavior / InfluenceFieldItem compile repair**, a green full-engine build is the required gate before further module rollout (Phase 2+ extractions).

## Standard full suite (matches `.gitlab-ci.yml`)

```bash
export CXX="${CXX:-g++}"   # Ubuntu cloud images: default c++ may lack libstdc++
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build --parallel
cd build && ctest --output-on-failure
./scripts/check-module-deps.sh
```

## Scoped repair (2026-11-05)

| Item | Detail |
|------|--------|
| **Symptom** | GCC 13: `std::vector<InfluenceFieldItem>` in `IBehavior.h` with forward-declared `InfluenceFieldItem` — incomplete type in vector destructor |
| **Fix** | `#include "../influence/InfluenceFieldItem.h"` in `core/interfaces/IBehavior.h`; manifest `core.interfaces` `requires` adds `core.influence` (patch 1.0.1) |
| **Follow-on** | `ImmunityManager.cpp` missing `#include <vector>` (latent; exposed once AI compiled) |
| **Regression** | `tests/UnitTests/AI/IBehaviorInterfaceTest.cpp` — default `GenerateFieldItems()` on a minimal `IBehavior` impl |

Behavior and public API unchanged; only include completeness and documented dependency.

## Stacked verification (open PRs)

When multiple agent branches are open (e.g. math pilot + build repair), validate on a **disposable local branch/worktree** by merging both heads — do not rewrite published PR branches.

```bash
git worktree add ../coreengine-stack main
cd ../coreengine-stack
git merge --no-edit origin/cursor/fix-full-build-ibehavior-vector-5517
git merge --no-edit origin/cursor/phase2-core-math-pilot-5517
# run full suite + math-only script from math PR
```

GitHub mirror PR merges do not sync to GitLab `gamedev4680743/coreengine`.

## Next module step (after gates)

With full CI green and math pilot merged: continue Phase 2 rollout to **`core.influence`** or **`core.logging`** (see [INVENTORY.md](INVENTORY.md) §5 runner-up) — one module at a time, same MODULE.md + checker opt-in pattern as `core.math`.
