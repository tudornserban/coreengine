# Module: `core.influence.types`

Manifest: [docs/agentic/modules/core.influence.types.module.json](../../docs/agentic/modules/core.influence.types.module.json)

## Purpose

Neutral **value-type leaf** for artificial potential field data: `InfluenceFieldItem`, parameter structs, and the single `.cpp` implementing item methods. No field force integration, no territory logic, no `IGameUnit`.

## Public surface (`exports`)

Prefer canonical includes from repo root:

```cpp
#include "core/influence_types/InfluenceFieldItem.h"
#include "core/influence_types/InfluenceFieldParameters.h"
```

Legacy compatibility (downward re-export only — **do not** use from `core.interfaces`):

```cpp
#include "core/influence/InfluenceFieldItem.h"
```

| Symbol | Header |
|--------|--------|
| `InfluenceFieldItem`, `InfluenceType` | `InfluenceFieldItem.h` |
| `AttractorParams`, `RepulsorParams`, `EdgeParams`, … | `InfluenceFieldParameters.h` |

## Dependencies (`requires`)

| Module | Range |
|--------|-------|
| `core.math` | `^1.0.0` |

## Tests

- `./scripts/run-influence-types-module-tests.sh` — types + math only (header compile TUs + `InfluenceTypesModuleTest.cpp`).
- Full engine and `./scripts/run-influence-module-tests.sh` also run `InfluenceFieldTest.cpp` (calculator/territory) plus the same header/module tests.

## Change checklist

- [ ] Bump manifest version when exports or observable behavior changes
- [ ] Run `./scripts/check-module-deps.sh` and `python3 scripts/test_check_module_deps.py`