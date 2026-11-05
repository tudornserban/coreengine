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
```

See [docs/agentic/BUILD_INTEGRATION.md](docs/agentic/BUILD_INTEGRATION.md).

## Phase 2 pilot

**`core.math`** — see [core/math/MODULE.md](core/math/MODULE.md) and [docs/agentic/PHASE2_PILOT.md](docs/agentic/PHASE2_PILOT.md).

Math-only verification:

```bash
./scripts/run-math-module-tests.sh
```

## Human docs

- [Architecture](docs/ARCHITECTURE.md)
- [Coding guidelines](docs/CODING_GUIDELINES.md)
- [Agentic index](docs/agentic/README.md)
