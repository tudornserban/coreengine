# Phase 2 pilot — `core.math`

**Status:** Complete (Phase 2 pilot on GitHub mirror; GitLab handoff manual).

## Goals

1. Add `core/math/MODULE.md` from [MODULE.md.template](../MODULE.md.template). **Done**
2. CMake OBJECT library `coreengine_math` + `COREENGINE_MATH_STANDALONE` math-only tests. **Done**
3. Extend `scripts/check-module-deps.sh` to fail on illegal `#include` for `core.math`. **Done**
4. Other modules: manifest validation only until opted in. **Documented**

## Success criteria

| Criterion | Met |
|-----------|-----|
| Agents can change `core/math/` + tests + manifest without sibling implementation context | Yes — see [core/math/MODULE.md](../../core/math/MODULE.md) |
| `./scripts/check-module-deps.sh` validates include boundaries for `core.math` | Yes |
| Math-only test path avoids compiling sibling `core/` implementation | Yes — `./scripts/run-math-module-tests.sh` |

## Verification commands

```bash
./scripts/check-module-deps.sh
python3 scripts/test_check_module_deps.py
./scripts/run-math-module-tests.sh
```

## Checker coverage / limitations

- **Enforced:** `core.math` sources under `core/math/` (`.h`, `.hpp`, `.cpp`).
- **Rules:** no `platform/` or `ffi/`; no `core/…` outside this module unless listed in manifest `requires` (empty today); quoted includes must resolve inside `core/math/`.
- **Not enforced yet:** all other registry modules (manifest + cycle check only); tests and `ffi/` trees; transitive includes through standard headers.

See [INVENTORY.md](../INVENTORY.md) §5 for rationale.