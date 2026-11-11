# Module: `core.logging`

Manifest: [docs/agentic/modules/core.logging.module.json](../../docs/agentic/modules/core.logging.module.json)

## Purpose

Foundation logging: levels, singleton `VenusWars::Logging::Logger`, optional file append, and compile-time `LOG_*` macros. Types live in namespace `VenusWars::Logging` (`Logger.h`, `LogLevel.h`). No gameplay, platform, or FFI dependencies.

## Public surface (`exports`)

```cpp
#include "core/logging/Logger.h"
#include "core/logging/LogLevel.h"
#include "core/logging/LoggerIntegration.h"  // optional integration hook macro
```

| Symbol | Header |
|--------|--------|
| `Logger`, `LOG_*` macros | `Logger.h` |
| `LogLevel`, `LogLevelToString` | `LogLevel.h` |
| `LoggerIntegration.h` | `SetupSDL2Logging()` when SDL headers present |

## Dependencies (`requires`)

| Module | Range |
|--------|-------|
| *(none)* | — |

## Tests

```bash
./scripts/run-logging-module-tests.sh
```

| File | Role |
|------|------|
| `tests/UnitTests/Logging/LoggerTest.cpp` | Runtime level threshold + isolated file output |
| `tests/UnitTests/Logging/LoggingLogLevelHeaderCompileTest.cpp` | `#include "core/logging/LogLevel.h"` only |
| `tests/UnitTests/Logging/LoggingCanonicalHeaderCompileTest.cpp` | `#include "core/logging/Logger.h"` only |
| `tests/UnitTests/Logging/LoggingIntegrationHeaderCompileTest.cpp` | `#include "core/logging/LoggerIntegration.h"` only |

## Change checklist

- [ ] Bump manifest patch/minor/major per [AGENTS.md](../../AGENTS.md)
- [ ] `./scripts/check-module-deps.sh` and `python3 scripts/test_check_module_deps.py`
- [ ] `./scripts/run-logging-module-tests.sh`