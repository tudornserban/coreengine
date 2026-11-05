# Phase 2 pilot — `core.math` (placeholder)

**Status:** Not started (Phase 1 scaffold only).

## Goals

1. Add `core/math/MODULE.md` from [MODULE.md.template](../MODULE.md.template).
2. Optional: CMake OBJECT library or explicit source list for `core.math` (keep linking into `coreengine` initially).
3. Extend `scripts/check-module-deps.sh` to fail on `#include` of paths outside `requires`.
4. Remove illegal includes in other modules only when touching those modules.

## Success criteria

- Agents can implement changes touching only `core/math/` + tests + manifest.
- `./scripts/check-module-deps.sh` validates include boundaries for `core.math`.

See [INVENTORY.md](../INVENTORY.md) §5 for rationale.
