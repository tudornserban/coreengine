# Agentic workflow (CoreEngine)

This folder holds **Phase 0 inventory** and **Phase 1 scaffold** for modular, agent-friendly development. It does not change runtime engine behavior.

| Document | Purpose |
|----------|---------|
| [INVENTORY.md](INVENTORY.md) | Module map, build/CI entry points, coupling notes |
| [manifest.schema.json](manifest.schema.json) | JSON Schema for per-module manifests |
| [modules/](modules/) | One manifest per logical module (`*.module.json`) |
| [MODULE.md.template](MODULE.md.template) | Copy when adding or splitting a module |
| [DEPENDENCY_POLICY.md](DEPENDENCY_POLICY.md) | Intended dependency layers (enforcement grows in Phase 2+) |
| [../deps/](../deps/) | Manifest-only `requires` graph (`graph.json`) and summary (`SUMMARY.md`), drift-gated by `check_module_deps.py` |
| [Interface changelog](../../core/interfaces/CHANGELOG.md) | SemVer log for `core.interfaces` (`core/interfaces/`); must be updated whenever that manifest version changes |
| [BUILD_INTEGRATION.md](BUILD_INTEGRATION.md) | Full-engine build gate and compile repair notes |
| [PHASE2_PILOT.md](PHASE2_PILOT.md) | `core.math` Phase 2 pilot (complete) |
| [PHASE2_INFLUENCE.md](PHASE2_INFLUENCE.md) | `core.influence` + `core.influence.types` Phase 2 |
| [PHASE2_LOGGING.md](PHASE2_LOGGING.md) | `core.logging` Phase 2 rollout |

**Phase 2:** `core.math` boundary documented in [core/math/MODULE.md](../../core/math/MODULE.md); include enforcement and math-only tests via `./scripts/run-math-module-tests.sh`.
