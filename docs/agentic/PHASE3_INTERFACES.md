# Phase 3 — `core.interfaces` include enforcement (A3.1)

**Status:** Include-path enforcement opt-in for the contracts module (headers under `core/interfaces/`). Manifest `requires` unchanged: `core.math`, `core.influence.types`. No public API or behavior change.

## Rationale

`core.interfaces` is header-only but still participates in the same `#include` boundary checks as foundation/systems modules once listed in `INCLUDE_ENFORCED_MODULES` in `scripts/check_module_deps.py`. Illegal sibling pulls (e.g. `core/influence/` without a manifest edge) fail `scripts/check-module-deps.sh`.

## Verification

Same commands as [BUILD_INTEGRATION.md](BUILD_INTEGRATION.md), plus interfaces-only:

```bash
./scripts/run-interfaces-module-tests.sh
```

That configures `COREENGINE_INTERFACES_STANDALONE=ON` and runs `InterfacesModuleHeaderCompileTest` (canonical `#include "core/interfaces/…"` compile smoke).

## SemVer

A3.1 is **enforcement and documentation only**; the `core.interfaces` manifest version was not bumped. Future contract changes still follow AGENTS.md patch/minor/major rules and `core/interfaces/CHANGELOG.md`.
