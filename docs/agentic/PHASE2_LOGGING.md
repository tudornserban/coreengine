# Phase 2 — `core.logging` rollout plan

**Status:** Implemented on PR (OBJECT lib + standalone tests + checker opt-in). Base `main` @ `7edc0bd`.

## Rationale (source-based)

| Fact | Evidence |
|------|----------|
| Small surface | One `.cpp` (`Logger.cpp`); headers `Logger.h`, `LogLevel.h`, `LoggerIntegration.h` (integration header is inline SDL hook only) |
| No upstream `core/` deps | `Logger.cpp` includes only `Logger.h` + standard library |
| Existing manifest | `core.logging` **1.0.0**, `requires: {}`, exports unchanged |
| Callers | `core/ai`, `core/collision`, `core/gamestate` include `../logging/Logger.h`; manifests already declare `requires.core.logging` on `core.ai` / `core.collision` |
| Tests today | **None** dedicated — new unit tests document observable level/file behavior without changing implementation |
| `requires` cycles | Full registry acyclic; logging adds no new edges |

## Boundary / ownership

- **OBJECT library** `coreengine_logging` owns `Logger.cpp` only.
- Remove `core/logging/*.cpp` from monolithic `CORE_SOURCES` glob; link `$<TARGET_OBJECTS:coreengine_logging>` into `coreengine` (same pattern as math/influence).
- **Unchanged:** include paths (`core/logging/…`), `VenusWars::Logging` types, macros, filtering (`level < m_currentLevel` skip), console/file split, singleton lifetime, no new threading layer.

## Implementation steps

1. `core/logging/CMakeLists.txt` + `core/logging/MODULE.md`
2. Root `CMakeLists.txt`: `COREENGINE_LOGGING_STANDALONE`, `add_subdirectory(core/logging)` in full + logging-standalone paths; PIC via existing `BUILD_FFI` global flag
3. Tests: `LoggerTest.cpp`, canonical + integration header compile TUs
4. `./scripts/run-logging-module-tests.sh`
5. Opt-in `core.logging` in `INCLUDE_ENFORCED_MODULES` + checker regressions
6. Manifest patch **1.0.1** (boundary/test rollout, no export change)
7. Narrow doc/CI: `DEPENDENCY_POLICY.md`, `BUILD_INTEGRATION.md`, `README.md`, `.gitlab-ci.yml` job mirroring math/influence pilots

## Validation surface (acceptance)

- Checker + regressions; full CTest FFI OFF/ON; `verify-ffi-build.sh`; math/types/influence/**logging** standalone; single `Logger.cpp.o` in archive; math standalone still one RNG object only.

## Stop conditions

- Baseline failure on `main` before branch work
- Manifest `requires` cycle introduced
- Need to change public logger API, macro names, or output semantics
- Duplicate `Logger.cpp` in link or forced allowlist for includes
