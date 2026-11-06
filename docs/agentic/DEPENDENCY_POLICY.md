# Module dependency policy (draft)

Enforcement is **active** for `core.math`, `core.influence.types`, `core.influence`, `core.logging`, and `core.interfaces` in Phase 2–3 (`scripts/check-module-deps.sh`). Other registry modules remain **manifest-only** until opted in. **`requires` graphs must be acyclic** — no checker exceptions.

The committed manifest-only **`requires` graph** (machine-readable JSON plus a human summary) lives under [`docs/deps/`](../deps/). `scripts/check_module_deps.py` regenerates and byte-compares these files on every run so committed artifacts cannot drift from the manifests.

## Layers

1. **Foundation:** `core.math`, `core.influence.types`, `core.logging`
2. **Contracts:** `core.interfaces` (may depend on foundation / value-type leaves — not on `core.influence` systems)
3. **Systems:** `core.ai`, `core.influence`, `core.collision`, `core.map`, `core.gameloop`, `core.combat`
4. **Integration:** `core.gamestate`, `core.serialization`
5. **Experimental:** `core.ai_advanced`

## Rules (target)

| Rule | Rationale |
|------|-----------|
| No `#include` from `core/` to `platform/` or `ffi/` | Platform-agnostic core |
| Systems must not `#include` concrete types from `core.gamestate/` except integration modules | Keeps agents scoped to one module |
| `core.collision` should depend on `IGameUnit`, not `RTSUnit` | Documented violation today; remove in Phase 2 pilot follow-ups |
| Depend only on modules listed in `requires` with satisfied semver | Agent + CI guardrail |
| No cycles in `requires` | Enables ordered extraction; strictly enforced |

## Version bumps

See [AGENTS.md](../../AGENTS.md) — patch/minor/major rules apply per module `version` in `docs/agentic/modules/*.module.json`.
