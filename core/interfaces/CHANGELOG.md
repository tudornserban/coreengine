# `core.interfaces` changelog

SemVer for this package is defined in `docs/agentic/modules/core.interfaces.module.json`.  
Agents **must** append a dated note under the matching `## [version]` section whenever that version changes (no silent manifest bumps).

Entries describe the **published contract**: manifest `exports` and the headers under `core/interfaces/`, plus manifest `requires` edges. Follow [AGENTS.md](../../AGENTS.md) for major / minor / patch rules.

## [1.1.0] — 2026-11-06

- **Export:** `IUnitKinematics.h` — optional write-only kinematics (`SetPosition`, `SetVelocity`) for systems such as collision resolution. `RTSUnit` implements this alongside `IGameUnit`; types that only expose `IGameUnit` are unchanged.

## [1.0.2] — 2026-11-06

**Initial changelog record for the current contract (no prior history consolidated here).**

- **Exports (manifest):** `IGameUnit`, `IBehavior`, `ITransitionCondition`, `IGameState`, `IGameLogicRenderer`, `IInputReader`, `ISerializer`, `IMapComponent` — pure virtual / contract headers in `core/interfaces/`.
- **Requires:** `core.math` `^1.0.0`, `core.influence.types` `^1.0.0` (types pulled by interface headers as needed).
- **Phase 3 (A3.1):** include-path enforcement enabled for `core.interfaces` in `scripts/check_module_deps.py`; added `core/interfaces/MODULE.md`, `InterfacesModuleHeaderCompileTest`, and `./scripts/run-interfaces-module-tests.sh`. SemVer unchanged (no API or behavior change).