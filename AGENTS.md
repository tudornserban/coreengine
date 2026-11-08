# CoreEngine — agent instructions

This repository uses an **additive, module-by-module** workflow. Phase 1 scaffold lives under `docs/agentic/`.

## Scope rules

1. **One module per task** unless the user explicitly asks for cross-cutting work.
2. **Path allowlists** — when editing module `core.<name>`, you may read/write:
   - `docs/agentic/modules/core.<name>.module.json`
   - Paths under that module’s `path` in the manifest (see [INVENTORY.md](docs/agentic/INVENTORY.md))
   - Matching tests under `tests/UnitTests/` or `tests/IntegrationTests/` for that area
   - Shared docs: `docs/agentic/*`, `docs/ARCHITECTURE.md` (if behavior changed)
3. **Do not read other modules’ implementation** for context when working a single module — use `core/interfaces` headers and the target module only. Exception: integration modules (`core.gamestate`, `core.serialization`) when the task requires it.
4. **No platform or FFI changes** when working `core/*` unless the task is platform/FFI scoped.
5. **No wide refactors** in inventory/scaffold PRs — extract interfaces in dedicated Phase 2+ PRs.

## Module metadata

- Manifest schema: [docs/agentic/manifest.schema.json](docs/agentic/manifest.schema.json)
- Registry: [docs/agentic/modules/registry.json](docs/agentic/modules/registry.json)
- Per-module semver: `version` field in each `*.module.json`

### Version bump rules

| Change | Bump |
|--------|------|
| Internal-only, no export/API change | Patch |
| New export or backward-compatible behavior | Minor |
| Remove/rename export or break callers | Major |

Update the module’s `*.module.json` in the same PR as the code change.

For **`core.interfaces`** (headers under `core/interfaces/`), treat the manifest **`exports`** list as the published API surface — the same table applies (add/remove/rename an export, or break implementers or callers, drives minor vs major; doc-only or internal manifest tweaks with no contract change can be patch). **Every** manifest version bump for `core.interfaces` must append an entry under the matching `## [version]` heading in [`core/interfaces/CHANGELOG.md`](core/interfaces/CHANGELOG.md). Do not bump the module version in a PR without updating that changelog; CI-style checks may enforce the version string is present. If you add a new published interface package later, add a semver-keyed changelog beside it and wire the same rule in docs.

## Verification

Full-engine gate (GitLab `build-and-test` job). Run from the **repository root**:

```bash
export CXX="${CXX:-g++}"
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./scripts/check-module-deps.sh
python3 scripts/test_check_module_deps.py
./scripts/run-math-module-tests.sh
./scripts/run-influence-module-tests.sh
./scripts/run-interfaces-module-tests.sh
./scripts/run-gameloop-module-tests.sh
./scripts/run-map-module-tests.sh
./scripts/run-ai-module-tests.sh
./scripts/run-serialization-module-tests.sh
./scripts/run-collision-module-tests.sh
```

**Include enforcement:** `./scripts/check-module-deps.sh` enforces `#include` boundaries for opted-in registry modules (see `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`), including `core.gameloop` as of Phase 3 A3.2, `core.map` as of Phase 3 A3.3, `core.ai` as of Phase 3 A3.4, `core.serialization` as of Phase 3 A3.5, `core.collision` as of R1.3 (A3.6), `core.combat` as of R1.6, and `core.gamestate` as of R2.1.

See [docs/agentic/BUILD_INTEGRATION.md](docs/agentic/BUILD_INTEGRATION.md).

## Phase 2 pilot

**`core.math`** — see [core/math/MODULE.md](core/math/MODULE.md) and [docs/agentic/PHASE2_PILOT.md](docs/agentic/PHASE2_PILOT.md).

Math-only verification:

```bash
./scripts/run-math-module-tests.sh
```

Influence-only verification:

```bash
./scripts/run-influence-module-tests.sh
```

Gameloop-only verification:

```bash
./scripts/run-gameloop-module-tests.sh
```

## Human docs

- [Architecture](docs/ARCHITECTURE.md)
- [Coding guidelines](docs/CODING_GUIDELINES.md)
- [Agentic index](docs/agentic/README.md)