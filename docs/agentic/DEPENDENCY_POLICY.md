# Module dependency policy (draft)

Enforcement is **active for `core.math`** in Phase 2 (`scripts/check-module-deps.sh` via `scripts/check_module_deps.py`). All other registry modules remain **manifest-only** until individually opted into `INCLUDE_ENFORCED_MODULES`.

## Layers

1. **Foundation:** `core.math`, `core.logging`
2. **Contracts:** `core.interfaces` (may depend on foundation)
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
| No cycles in `requires` | Enables ordered extraction |

**Exception (documented):** `core.interfaces` ↔ `core.influence` mutual `requires` — `IBehavior` returns `InfluenceFieldItem`; `TerritoryCalculator` uses `IGameUnit`. Allowed by `scripts/check_module_deps.py` only for this pair.

## Version bumps

See [AGENTS.md](../../AGENTS.md) — patch/minor/major rules apply per module `version` in `docs/agentic/modules/*.module.json`.
