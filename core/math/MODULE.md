# Module: `core.math`

Manifest: [docs/agentic/modules/core.math.module.json](../../docs/agentic/modules/core.math.module.json)

## Purpose

Foundation math for CoreEngine: 2D/3D vectors, shared constants, and a deterministic pseudo-random generator for replay-safe simulation. This module does **not** own gameplay rules, collision, rendering, or platform adapters.

## Public surface (`exports`)

Include headers from the repo root, e.g. `#include "core/math/Vector2.h"`.

| Symbol | Kind | Header | Notes |
|--------|------|--------|-------|
| `Vector2` | struct | `Vector2.h` | 2D vector algebra; header-only |
| `Vector3` | struct | `Vector3.h` | 3D vector algebra; header-only |
| `MathConstants` | namespace `CoreEngine::Math` | `MathConstants.h` | `PI`, angle conversion, `EPSILON`, float limits |
| `DeterministicRNG` | class | `DeterministicRNG.h` | Seeded LCG; `.cpp` in this module |

### Contract highlights

- **Determinism:** `DeterministicRNG` with the same seed produces the same sequence on all platforms (32-bit LCG step; floats/doubles derived from `NextUInt32()`).
- **Ranges:** `NextFloat()` / `NextDouble()` are in **[0, 1)**; ranged overloads use **[min, max)**; `NextInt32(min, max)` is **inclusive** on both ends; if `min >= max`, ranged overloads return `min`.
- **Vectors:** `Normalized()` / `Normalize()` on zero-length vectors yield zero; `ClampMagnitude` only scales down when length exceeds the cap.
- **No hidden state:** value types and RNG instance state only; no globals.

## Dependencies (`requires`)

| Module | Range | Why |
|--------|-------|-----|
| *(none)* | — | Foundation layer; only standard library headers |

Allowed `#include` in `core/math/**`: standard library (`<…>`), headers under `core/math/`, or quoted includes that resolve inside `core/math/`. No `core/*` sibling modules, `platform/`, or `ffi/` (enforced by `./scripts/check-module-deps.sh`).

## Agent edit scope

| Allow | Path |
|-------|------|
| Implementation | `core/math/**` |
| Manifest | `docs/agentic/modules/core.math.module.json` |
| Unit tests | `tests/UnitTests/Math/**` |
| Shared agentic docs | `docs/agentic/*` (when policy requires) |

Do not read other `core/*` implementation trees for math-only tasks.

## Invariants

- Preserve LCG constants and stepping logic unless intentionally breaking semver major.
- Do not introduce `#include` of other CoreEngine modules without updating manifest `requires` and dependency policy.
- Keep public includes stable at `core/math/…` (repo-root include path).

## Tests

| Suite | Path |
|-------|------|
| RNG | `tests/UnitTests/Math/DeterministicRNGTest.cpp` |
| Constants | `tests/UnitTests/Math/MathConstantsTest.cpp` |
| Vector2 | `tests/UnitTests/Math/Vector2Test.cpp` |
| Vector3 | `tests/UnitTests/Math/Vector3Test.cpp` |

### Math-only (no sibling `core/` `.cpp` compiled)

```bash
chmod +x scripts/run-math-module-tests.sh
./scripts/run-math-module-tests.sh
```

Equivalent manual steps:

```bash
cmake -S . -B build-math -G Ninja -DBUILD_TESTS=ON -DCOREENGINE_MATH_STANDALONE=ON
cmake --build build-math --target coreengine_math_tests
cd build-math && ctest --output-on-failure -R 'DeterministicRNGTest|MathConstantsTest|Vector2Test|Vector3Test'
```

### Full engine (links `coreengine` as today)

```bash
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure -R 'DeterministicRNGTest|MathConstantsTest|Vector2Test|Vector3Test'
```

## Change checklist

- [ ] Update `docs/agentic/modules/core.math.module.json` version if exports or observable behavior changes
- [ ] Update this file if boundaries or commands change
- [ ] Run `./scripts/check-module-deps.sh`
- [ ] Run `./scripts/run-math-module-tests.sh` (or full test command above)
