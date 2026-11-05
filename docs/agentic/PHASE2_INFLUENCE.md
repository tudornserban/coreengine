# Phase 2 — `core.influence` (scoped plan)

**Status:** Complete (pending PR review).  
**Base:** `main` @ `628ce572` (math pilot + build repair merged).

## Goal

Apply the **same pattern as `core.math`**: MODULE contract, honest manifest, include checker opt-in, isolated test target, lightweight GitLab job — without API/behavior changes or sibling refactors.

## Dependency facts (from code)

| Source | `#include` / use | Manifest `requires` |
|--------|------------------|---------------------|
| `InfluenceField*.h/.cpp` | `core/math/Vector2.h`, stdlib | `core.math` |
| `TerritoryCalculator.cpp` | `core/interfaces/IGameUnit.h` | `core.interfaces` |
| `TerritoryCalculator.h` | `IGameUnit*` parameters only | forward declare in header; full type in `.cpp` |
| `core/interfaces/IBehavior.h` (already on `main`) | `core/influence/InfluenceFieldItem.h` | `core.interfaces` → `core.influence` |

**Mutual manifest edge:** `core.interfaces` ↔ `core.influence` is intentional (contract API uses `InfluenceFieldItem`; territory uses `IGameUnit`). Documented exception to strict DAG — checker allows this pair only.

No other core modules are included from `core/influence/**`.

## Deliverables

1. `core/influence/MODULE.md` — ownership, exports, dependencies, agent scope, commands.
2. `core/influence/CMakeLists.txt` — `coreengine_influence` OBJECT library; explicit `.cpp` list.
3. Root `CMakeLists.txt` — `COREENGINE_INFLUENCE_STANDALONE` test executable (links `coreengine_influence` + `coreengine_math` objects only; **not** full `coreengine`).
4. `scripts/run-influence-module-tests.sh` + GitLab `influence-module-pilot` job.
5. `scripts/check_module_deps.py` — enforce includes for `core.influence`; keep `core.math`; mutual-cycle exception for interfaces↔influence.
6. `scripts/test_check_module_deps.py` — influence positive/negative cases (sibling + relative escape).
7. Manifest patch: `core.influence` `requires` adds `core.interfaces`; notes updated.
8. Optional focused test only if gap found (existing `InfluenceFieldTest.cpp` retained).

## Non-goals

- No GUI, no Phase 3 rollout, no collision/gamestate refactors.
- No weakening checker or skipping production tests.

## Verification (repository root)

```bash
export CXX="${CXX:-g++}"
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON && cmake --build build --parallel
ctest --test-dir build --output-on-failure
./scripts/check-module-deps.sh && python3 scripts/test_check_module_deps.py
./scripts/run-math-module-tests.sh
./scripts/run-influence-module-tests.sh
```

## Success criteria

- Influence changes scoped to `core/influence/`, its tests, manifests, checker/CI/docs.
- Standalone influence tests compile **only** math + influence object units (+ test helpers).
- Full suite remains green (235+ tests).
