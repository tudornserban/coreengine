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
| [PHASE3_INTERFACES.md](PHASE3_INTERFACES.md) | `core.interfaces` include enforcement (A3.1) |
| [PHASE3_GAMELOOP.md](PHASE3_GAMELOOP.md) | `core.gameloop` include enforcement (A3.2) |
| [PHASE3_MAP.md](PHASE3_MAP.md) | `core.map` include enforcement (A3.3) |
| [PHASE3_AI.md](PHASE3_AI.md) | `core.ai` include enforcement (A3.4) |
| [PHASE3_SERIALIZATION.md](PHASE3_SERIALIZATION.md) | `core.serialization` include enforcement (A3.5) |

**Phase 2:** `core.math` boundary documented in [core/math/MODULE.md](../../core/math/MODULE.md); include enforcement and math-only tests via `./scripts/run-math-module-tests.sh`.

**Phase 3:** `core.interfaces` boundary documented in [core/interfaces/MODULE.md](../../core/interfaces/MODULE.md); header compile smoke test via `./scripts/run-interfaces-module-tests.sh`. `core.gameloop` is documented in [core/gameloop/MODULE.md](../../core/gameloop/MODULE.md); gameloop-only build via `./scripts/run-gameloop-module-tests.sh`. `core.map` is documented in [core/map/MODULE.md](../../core/map/MODULE.md); map-only build via `./scripts/run-map-module-tests.sh`. `core.ai` is documented in [core/ai/MODULE.md](../../core/ai/MODULE.md); AI-only build via `./scripts/run-ai-module-tests.sh`. `core.serialization` is documented in [core/serialization/MODULE.md](../../core/serialization/MODULE.md); serialization-only build via `./scripts/run-serialization-module-tests.sh`.