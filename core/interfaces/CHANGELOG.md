# `core.interfaces` changelog

SemVer for this package is defined in `docs/agentic/modules/core.interfaces.module.json`.  
Agents **must** append a dated note under the matching `## [version]` section whenever that version changes (no silent manifest bumps).

Entries describe the **published contract**: manifest `exports` and the headers under `core/interfaces/`, plus manifest `requires` edges. Follow [AGENTS.md](../../AGENTS.md) for major / minor / patch rules.

## [1.0.2] — 2025-11-06

**Initial changelog record for the current contract (no prior history consolidated here).**

- **Exports (manifest):** `IGameUnit`, `IBehavior`, `ITransitionCondition`, `IGameState`, `IGameLogicRenderer`, `IInputReader`, `ISerializer`, `IMapComponent` — pure virtual / contract headers in `core/interfaces/`.
- **Requires:** `core.math` `^1.0.0`, `core.influence.types` `^1.0.0` (types pulled by interface headers as needed).
